#include <linux/module.h>
#include <linux/init.h>
#include <linux/serial_8250.h>
#include <linux/serial_reg.h>
#include <linux/serial_core.h>
#include <linux/tty_flip.h>
#include "bspchip.h"
#include "bspchip_9607c.h"

#define map_8250_out_reg(up, offset) (offset)

#ifdef CONFIG_RTK_NETLOG
static struct uart_port *netlog_handle = NULL;
static void plat_serial_out(struct uart_port *p, int offset, int value)
{
	offset = map_8250_out_reg(p, offset) << p->regshift;

	writeb(value, p->membase + offset);
	
	if (unlikely(netlog_handle==NULL)) {
		netlog_handle = p;
	}
	
	if(UART_TX == offset)
	{		
		extern void netlog_emit_char(char);
		netlog_emit_char((char)value);
	}
}

void console_input(const char *buf, int buflen) {
	int i;
	
	if (netlog_handle) {
		struct tty_port *tport = &netlog_handle->state->port;
		for (i=0; i<buflen; i++) 
			uart_insert_char(netlog_handle, 0, 0, buf[i], TTY_NORMAL);

		tty_flip_buffer_push(tport);
	} 	
}
#endif 

static struct plat_serial8250_port uart8250_data[] = {
	{
		.membase	= (unsigned int*)_UART_BASE,
		.mapbase    = _UART_BASE & 0x1fffffff,
		.irq		= _UART_IRQ,
		.uartclk	= 0,
		.type       = PORT_16550A,
		.iotype		= UPIO_MEM,
		.flags		= UPF_SKIP_TEST | UPF_FIXED_TYPE,
		.regshift	= 2,
		#ifdef CONFIG_RTK_NETLOG
		.serial_out = plat_serial_out,
		#endif 
	},
	{ },
};

static struct platform_device uart8250_device = {
	.name			= "serial8250",
	.id			= PLAT8250_DEV_PLATFORM,
	.dev			= {
		.platform_data	= uart8250_data,
	},
};

static int __init uart8250_init(void)
{
	uart8250_data[0].uartclk = BSP_SYSCLK - BSP_BAUDRATE * 24;
	return platform_device_register(&uart8250_device);
}

module_init(uart8250_init);

MODULE_AUTHOR("Andrew Chang <yachang@realtek.com>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Serial Driver");
