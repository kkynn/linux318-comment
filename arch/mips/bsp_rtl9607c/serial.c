/*
 * Realtek Semiconductor Corp.
 *
 * bsp/serial.c
 *     BSP serial port initialization
 *
 * Copyright 2006-2012 Tony Wu (tonywu@realtek.com)
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/serial.h>
#include <linux/serial_core.h>
#include <linux/serial_8250.h>
#include <linux/serial_reg.h>
#include <linux/tty.h>
#include <linux/irq.h>
#include <asm/io.h>
#include <asm/serial.h>
#include "bspchip.h"
#include <linux/serial_8250.h>
#include <linux/serial_core.h>
#if 0
// #if LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0)
unsigned int last_lcr;

unsigned int dwapb_serial_in(struct uart_port *p, int offset)
{
	offset <<= p->regshift;
	return readl(p->membase + offset);
}

void dwapb_serial_out(struct uart_port *p, int offset, int value)
{
	int save_offset = offset;
	offset <<= p->regshift;

	/* Save the LCR value so it can be re-written when a
	 * Busy Detect interrupt occurs. */
	if (save_offset == UART_LCR) {
		last_lcr = value;
	}
	writel(value, p->membase + offset);
	/* Read the IER to ensure any interrupt is cleared before
	 * returning from ISR. */
	if (save_offset == UART_TX || save_offset == UART_IER)
		value = p->serial_in(p, UART_IER);
}

static int dwapb_serial_irq(struct uart_port *p)
{
	unsigned int iir = readl(p->membase + (UART_IIR << p->regshift));

	if (serial8250_handle_irq(p, iir)) {
		return 1;
	} else if ((iir & UART_IIR_BUSY) == UART_IIR_BUSY) {
		/*
		 * The DesignWare APB UART has an Busy Detect (0x07) interrupt
		 * meaning an LCR write attempt occurred while the UART was
		 * busy. The interrupt must be cleared by reading the UART
		 * status register (USR) and the LCR re-written.
		 */
		(void)readl(p->membase + 0xc0);
		writel(last_lcr, p->membase + (UART_LCR << p->regshift));
		return 1;
	}

	return 0;
}
#endif

int prom_putchar(char c)
{
	unsigned int busy_cnt = 0;

#ifdef CONFIG_RTL867X_NETLOG
	netlog_emit_char(c);
#endif

	do
	{
		/* Prevent Hanging */
		if (busy_cnt++ >= 30000)
		{
			/* Reset Tx FIFO */
			REG8(_UART_FCR) = BSP_TXRST | BSP_CHAR_TRIGGER_14;
			return 0;
		}
	} while ((REG8(_UART_LSR) & BSP_LSR_THRE) == BSP_TxCHAR_AVAIL);

	/* Send Character */
	REG8(_UART_THR) = c;

	return 1;
}

int __init plat_serial_init(void)
{
#ifdef CONFIG_SERIAL_8250

#if (CONFIG_SERIAL_8250_RUNTIME_UARTS > CONFIG_SERIAL_8250_NR_UARTS)
#error "Insufficient UART PORTS"
#endif

	struct uart_port s;
	memset(&s, 0, sizeof(s));
		
	s.type = PORT_16550A;

#ifdef CONFIG_USE_UART0
 	s.mapbase = BSP_UART0_PADDR;
 	s.irq = BSP_UART0_IRQ;
#elif defined CONFIG_USE_UART1 	
	s.mapbase = BSP_UART1_PADDR;
	s.irq = BSP_UART1_IRQ;
#endif  		
	s.membase = ioremap_nocache(s.mapbase, BSP_UART0_PSIZE);

	s.uartclk = BSP_SYSCLK - BAUDRATE * 24;
	s.flags = UPF_SKIP_TEST | UPF_LOW_LATENCY | UPF_SPD_CUST;
	s.iotype = UPIO_MEM;
	s.regshift = 2;
	s.fifosize = 1;
	s.custom_divisor = BSP_SYSCLK / (BAUDRATE * 16) - 1;
	
	/* Call early_serial_setup() here, to set up 8250 console driver */
	if (early_serial_setup(&s) != 0) {
		panic("plat_serial_init: serial initialization failed!");
	}

#if (CONFIG_SERIAL_8250_RUNTIME_UARTS >= 2)

#if (defined CONFIG_USE_UART0 && defined CONFIG_CPU1_UART0) || (defined CONFIG_USE_UART1 && defined CONFIG_CPU1_UART1)
#error "Physical UART port conflict, please check settings (CONFIG_USE_UART? & CONFIG_CPU1_UART? & CONFIG_SERIAL_8250_RUNTIME_UARTS)"
#endif  		
	/* /dev/ttyS1 */
	s.line = 1;
	s.type = PORT_16550A;

#ifdef CONFIG_USE_UART0
	s.mapbase = BSP_UART1_PADDR;
	s.irq = BSP_UART1_IRQ;
#elif defined CONFIG_USE_UART1 	
	s.mapbase = BSP_UART0_PADDR;
	s.irq = BSP_UART0_IRQ;
#endif  		
	s.membase = ioremap_nocache(s.mapbase, BSP_UART0_PSIZE);
	
	s.uartclk = BSP_SYSCLK - BAUDRATE * 24;
	s.flags = UPF_SKIP_TEST | UPF_LOW_LATENCY | UPF_SPD_CUST;
	s.iotype = UPIO_MEM;
	s.regshift = 2;
	s.fifosize = 1;
	s.custom_divisor = BSP_SYSCLK / (BAUDRATE * 16) - 1;
	
	/* Call early_serial_setup() here, to set up 8250 console driver */
	if (early_serial_setup(&s) != 0) {
		panic("plat_serial_init: ttyS1 serial initialization failed!");
	}
#endif	/* CONFIG_SERIAL_8250_RUNTIME_UARTS >= 2 */

#if (CONFIG_SERIAL_8250_RUNTIME_UARTS >= 3)
	/* /dev/ttyS2 */
	REG32(0xb800220c) = 0x83000000;
	REG32(0xb8002200) = 0x6c000000;
	REG32(0xb800220c) = 0x03000000;

	s.line = 2;
	s.type = PORT_16550A;

	s.mapbase = BSP_UART2_PADDR;
	s.irq = BSP_UART2_IRQ;
	s.membase = ioremap_nocache(s.mapbase, BSP_UART2_PSIZE);
	
	s.uartclk = BSP_SYSCLK - BAUDRATE * 24;
	s.flags = UPF_SKIP_TEST | UPF_LOW_LATENCY | UPF_SPD_CUST;
	s.iotype = UPIO_MEM;
	s.regshift = 2;
	s.fifosize = 1;
	s.custom_divisor = BSP_SYSCLK / (BAUDRATE * 16) - 1;
	
	/* Call early_serial_setup() here, to set up 8250 console driver */
	if (early_serial_setup(&s) != 0) {
		panic("plat_serial_init: ttyS1 serial initialization failed!");
	}

#endif	/* CONFIG_SERIAL_8250_RUNTIME_UARTS >= 3 */

#endif	/* CONFIG_SERIAL_8250 */

	return 0;
}

device_initcall(plat_serial_init);

