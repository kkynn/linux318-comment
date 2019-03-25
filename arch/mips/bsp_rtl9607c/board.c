#include <linux/module.h>
#include <linux/kernel.h>
#include <rtk/init.h>
#include <rtk/i2c.h>
#include <rtk/led.h>
#include "bspgpio.h"
#include "led-generic.h"
#include "pushbutton.h"

#define PCIE0_RESET_PIN GPIO_40
#define PCIE1_RESET_PIN GPIO_39

void PCIE_reset_pin(int *reset){
	*reset = PCIE0_RESET_PIN;
	printk("PCIE0 reset pin is set to GPIO %d\n", *reset);
}
EXPORT_SYMBOL(PCIE_reset_pin);

#ifdef CONFIG_USE_PCIE_SLOT_1
void PCIE1_reset_pin(int *reset){
	*reset = PCIE1_RESET_PIN;
	printk("PCIE1 reset pin is set to GPIO %d\n", *reset);
}
EXPORT_SYMBOL(PCIE1_reset_pin);
#endif


#define LOW_ACTIVE	1
#if LOW_ACTIVE

#define GPIO_SET(w, op)  do { \
	gpioConfig(w, GPIO_FUNC_OUTPUT); \
	if (LED_ON==op) gpioClear(w); \
	else gpioSet(w); \
} while (0);

#define GPIO_READ(w) (!gpioRead(w))
		
#else

#define GPIO_SET(w, op)  do { \
	gpioConfig(w, GPIO_FUNC_OUTPUT); \
	if (LED_ON==op) gpioSet(w); \
	else gpioClear(w); \
} while (0);

#define GPIO_READ(w) (gpioRead(w))

#endif

static void board_01_handle_set(int which, int op) {
	//printk("%s: led %d op %d\n", __FUNCTION__, which, op);
	switch (which) {
	//case LED_POWER_RED:
	case LED_POWER_GREEN:
		GPIO_SET(GPIO_60, op);
		break;
#ifdef CONFIG_WIFI_LED_USE_SOC_GPIO
	case LED_WIFI:
		GPIO_SET(GPIO_7, op);
		break;
#endif
/* current only wifi led 
#if defined (CONFIG_RTL8192CD) || defined (CONFIG_RTL8192CD_MODULE)
        case LED_WPS_GREEN:
	case LED_WPS_YELLOW:
	case LED_WPS_RED:
                GPIO_SET(GPIO_7, op);
                break;
#endif 
*/
	default:
		led_handle_set(which, op);
	}
}

static void board_01_handle_init(void) {
	board_01_handle_set(LED_POWER_GREEN, LED_OFF);
	board_01_handle_set(LED_POWER_RED, LED_ON);
	board_01_handle_set(LED_DSL, LED_OFF);
	board_01_handle_set(LED_INTERNET_GREEN, LED_OFF);
	board_01_handle_set(LED_INTERNET_RED, LED_OFF);
	board_01_handle_set(LED_WPS_GREEN, LED_OFF);
	board_01_handle_set(LED_WPS_RED, LED_OFF);
	board_01_handle_set(LED_WPS_YELLOW, LED_OFF);
#ifdef CONFIG_SW_USB_LED1
	board_01_handle_set(LED_USB_1, LED_OFF);
#endif //CONFIG_SW_USB_LED1
#ifdef CONFIG_SW_USB_LED0
	board_01_handle_set(LED_USB_0, LED_OFF);
#endif //CONFIG_SW_USB_LED0

};

static struct led_operations board_01_operation = {
	.name = "board_01",
	.handle_init = board_01_handle_init,
	.handle_set = board_01_handle_set,
};


static void board_01_pb_init(void) {
};

static int board_01_pb_is_pushed(int which) {
	switch(which) {
	case PB_RESET:
		return GPIO_READ(GPIO_37);
	 case PB_WPS:
		return GPIO_READ(GPIO_34);

	default:
		return 0;
	}
}

static struct pushbutton_operations board_01_pb_op = {
	.handle_init = board_01_pb_init,
	.handle_is_pushed = board_01_pb_is_pushed,
};

static int __init board_01_led_init(void) {

#ifdef CONFIG_PCIE_POWER_SAVING
	REG32(IO_MODE_EN_REG) |= IO_MODE_INTRPT1_EN; // ENABLE interrupt 1 
#endif

	//set for WIFI
//	gpioConfig(PCIE0_RESET_PIN, GPIO_FUNC_OUTPUT);
//	gpioSet(PCIE0_RESET_PIN);
//#ifdef CONFIG_USE_PCIE_SLOT_1
//	gpioConfig(PCIE1_RESET_PIN, GPIO_FUNC_OUTPUT);
//	gpioSet(PCIE1_RESET_PIN);
//#endif	

    {
        rtk_led_config_t ledConfig;

        rtk_core_init();
        rtk_i2c_init(0);
        rtk_led_init();
        rtk_led_operation_set(LED_OP_PARALLEL);
        rtk_led_parallelEnable_set(2,ENABLED);	// port 0
        rtk_led_parallelEnable_set(3,ENABLED); // port 1
        rtk_led_parallelEnable_set(17,ENABLED); // port 2
        rtk_led_parallelEnable_set(5,ENABLED); // port 3
		rtk_led_parallelEnable_set(16,ENABLED); // port 4
        rtk_led_parallelEnable_set(4,ENABLED); // port 5
	rtk_led_parallelEnable_set(9,ENABLED); //LOS

        memset(&ledConfig,0x0,sizeof(rtk_led_config_t));    
        ledConfig.ledEnable[LED_CONFIG_SPD1000ACT] = ENABLED;
        ledConfig.ledEnable[LED_CONFIG_SPD500ACT] = ENABLED;
        ledConfig.ledEnable[LED_CONFIG_SPD100ACT] = ENABLED;
        ledConfig.ledEnable[LED_CONFIG_SPD10ACT] = ENABLED;
        ledConfig.ledEnable[LED_CONFIG_SPD1000] = ENABLED;
        ledConfig.ledEnable[LED_CONFIG_SPD500] = ENABLED;
        ledConfig.ledEnable[LED_CONFIG_SPD100] = ENABLED;
        ledConfig.ledEnable[LED_CONFIG_SPD10] = ENABLED;

        rtk_led_config_set(2,LED_TYPE_UTP0,&ledConfig);
        rtk_led_config_set(3,LED_TYPE_UTP1,&ledConfig);
        rtk_led_config_set(17,LED_TYPE_UTP2,&ledConfig);
        rtk_led_config_set(5,LED_TYPE_UTP3,&ledConfig);
        rtk_led_config_set(16,LED_TYPE_UTP4,&ledConfig);

        rtk_led_config_set(4,LED_TYPE_FIBER,&ledConfig);
	rtk_led_config_set(9,LED_TYPE_PON,&ledConfig);
    }

	led_register_operations(&board_01_operation);
	pb_register_operations(&board_01_pb_op);
	return 0;
}

static void __exit board_01_led_exit(void) {
}


module_init(board_01_led_init);
module_exit(board_01_led_exit);


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("GPIO driver for Reload default");

