#include <linux/kernel.h>
#include <linux/console.h>
#include <linux/printk.h>
#include <linux/init.h>
#include <linux/serial_8250.h>
#include <linux/serial_reg.h>

#include <asm/setup.h>
#include "bspchip.h"

#define CONFIG_HS_UART_NUM 5

#define O_DLL (0x0)
#define O_THR (0x0)
#define O_RBR (0x0)
#define O_DLH (0x4)
#define O_IER (0x4)
#define O_IIR (0x8)
	#define IIR_FIFO16_MASK (0xe0)
#define O_FCR (0x8)
	#define FCR_FIFO_EN (0x01)
	#define FCR_RBR_RESET (0x02)
	#define FCR_THR_RESET (0x04)
	#define FCR_RBR_TRIG_HI (0xc0)
#define O_ADD (0x8)
#define O_LCR (0xc)
	#define LCR_7B (0x2)
	#define LCR_8B (0x3)
	#define LCR_1S (0 << 2)
	#define LCR_2S (1 << 2)
	#define LCR_NO_PARITY (0 << 3)
	#define LCR_BRK_SIG (1 << 6)
	#define LCR_DLAB_EN (1 << 7)
#define O_MCR (0x10)
	#define MCR_DTR (0x1)
	#define MCR_RTS (0x2)
	#define MCR_AFE (0x20)
	#define MCR_CLK_LX (0x40)
#define O_LSR (0x14)
	#define LSR_TEMT (0x40)

/* #define REG8(addr) (*((volatile uint8_t *)addr)) */
#define UART_ADDR(id, off) (0xb8002000 + id*0x100 + off)
#define UART_REG(id, off) REG8(UART_ADDR(id, off))

#if defined(CONFIG_EARLY_PRINTK)
/* There are 2 methods to enable early console.
	 #1 is by typical 8250 framework. For example,
        setup_early_serial8250_console("uart8250,mmio32,0x18002000");
   Furthermore, serial8250_early_out() and serial8250_early_in() have to be impl. to
	 override the ones declared in ./driver/tty/serial/8250/8250_early.c

	 #2 is to hook with ./arch/mips/kernel/early_printk.c
	 In this case, just impl. prom_putchar() to link with setup_early_printk().
	 Which is the selected method for this project.
 */

/* Early printk doesn't setup UART registers and rely on loader to configure */
void prom_putchar(char c) {
	uint32_t busy_cnt = 86000;
	uint32_t uid = 0;

#ifdef CONFIG_RTL867X_NETLOG
	netlog_emit_char(c);
#endif

	while (((UART_REG(uid, O_LSR) & LSR_TEMT) == 0) && --busy_cnt);

	if (busy_cnt) {
		UART_REG(uid, O_THR) = c;
	} else {
		UART_REG(uid, O_FCR) = (FCR_RBR_TRIG_HI|FCR_THR_RESET|FCR_RBR_RESET|FCR_FIFO_EN);
	}

	return;
}
#endif

/* Defined at ./arch/mips/<BSP>/setup.c */
extern struct plat_serial8250_port hs_uart_port[];
static struct platform_device hs_uart_device = {
	.name = "serial8250",
	.id = PLAT8250_DEV_PLATFORM,
	.dev = {
		.platform_data = hs_uart_port,
	},
};

typedef void (set_termios)(struct uart_port *,struct ktermios *, struct ktermios *);

static void
hs_uart_set_termios(struct uart_port *port,
                    struct ktermios *termios, struct ktermios *old) {
	uint32_t baud;
	uint16_t quot;
	unsigned long flags;
	uint8_t add;

	struct uart_8250_port *up = container_of(port, struct uart_8250_port, port);

	const uint32_t bsp_sysclk = BSP_MHZ * 1000 * 1000;

	serial8250_do_set_termios(port, termios, old);

	baud = uart_get_baud_rate(port, termios, old,
	                          9600, 4000000);

	/* quot = uart_get_divisor(port, baud); */
	quot = bsp_sysclk / (16 * baud);

	add = (bsp_sysclk + (baud / 2))/baud - (16 * quot);

	spin_lock_irqsave(&port->lock, flags);

	REG8(port->membase + O_LCR) = up->lcr | LCR_DLAB_EN;
	REG8(port->membase + O_DLL) = quot & 0xff;
	REG8(port->membase + O_DLH) = quot >> 0x8;
	REG8(port->membase + O_ADD) = add;
	REG8(port->membase + O_LCR) = up->lcr;

	spin_unlock_irqrestore(&port->lock, flags);

	if (tty_termios_baud_rate(termios))
		tty_termios_encode_baud_rate(termios, baud, baud);

	return;
}

static set_termios*
get_set_termios_ptr(uint32_t uid) {
	uint8_t lcr_bak, is_hs_uart;

	lcr_bak = UART_REG(uid, O_LCR);
	UART_REG(uid, O_LCR) = (lcr_bak | LCR_DLAB_EN);
	is_hs_uart = ((UART_REG(uid, O_IIR) & IIR_FIFO16_MASK) == 0);
	UART_REG(uid, O_LCR) = lcr_bak;

	if (is_hs_uart) {
		return hs_uart_set_termios;
	} else {
		return NULL;
	}
}

static int __init hs_uart_init(void) {
	int i;

	i = 0;
	while (hs_uart_port[i].irq) {
		hs_uart_port[i].membase = (unsigned int *)(UART_ADDR(i, 0));
		hs_uart_port[i].mapbase = (UART_ADDR(i, 0)) & 0x1fffffff;
		hs_uart_port[i].uartclk = BSP_MHZ * 1000 * 1000;
		hs_uart_port[i].type = PORT_16550A;
		hs_uart_port[i].iotype = UPIO_MEM;
		hs_uart_port[i].flags = UPF_SKIP_TEST | UPF_FIXED_TYPE;
		hs_uart_port[i].regshift = 2;

		hs_uart_port[i].set_termios = get_set_termios_ptr(i);

		printk(KERN_INFO "UART%d set_termios @ %p\n", i, hs_uart_port[i].set_termios);
		i++;
	}

	return platform_device_register(&hs_uart_device);
}

device_initcall(hs_uart_init);
