#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <asm/processor.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <bspchip.h>
#include "led-generic.h"
#include "pushbutton.h"

//WPS MODE
#define GENERIC_MODEL_WPS		0
#define BIG_MODEL_WPS 			1
#define ALPHA_MODEL_WPS		2
#define E8B_MODEL_WPS			3

#define WPS_MODE				E8B_MODEL_WPS

typedef enum WPS_LED_STATE
{
	WPS_LED_INPROG_BLINK=0,
	WPS_LED_ERROR_BLINK=1,
	WPS_LED_OVERLAP_BLINK=2,
	WPS_LED_STOP_BLINK=3
} WPS_LED_STATE_t;


static WPS_LED_STATE_t AutoCfg_LED_Toggle_Mode; //blink state
static unsigned int AutoCfg_LED_Toggle_Step = 0; //current blink step
//cathy
static unsigned char wps_wlan_restart=0;

#if ( WPS_MODE == E8B_MODEL_WPS )
	/*every 0.1 second*/
	#define InprogBlinkTypeStep	3
	int InprogBlinkType[InprogBlinkTypeStep]={ 1,1,0 };
	#define ErrorBlinkTypeStep	2
	int ErrorBlinkType[ErrorBlinkTypeStep]={ 1,0 };
	#define OverlapBlinkTypeStep	15
	int OverlapBlinkType[OverlapBlinkTypeStep]={	1,0,1,0,1,
						 	0,1,0,1,0,
						 	0,0,0,0,0 };
#else //for default
	/*every 50 or 100 ticks*/
	#define InprogBlinkTypeStep	2
	int InprogBlinkType[InprogBlinkTypeStep]={ 1,0 };
	#define ErrorBlinkTypeStep	2
	int ErrorBlinkType[ErrorBlinkTypeStep]={ 1,0 };
	#define OverlapBlinkTypeStep	2
	int OverlapBlinkType[OverlapBlinkTypeStep]={ 1,0 };
#endif //( WPS_MODE == E8B_MODEL_WPS )

static struct timer_list wps_led_timer;	

/*
	dongqifan 20180116
	由于GP744GV/GP624GV2/CATV板子 wps led做反了， 只好重新定义led_on led_off
 */
void led_on_wps(int which)
{
	led_off(which);
}
void led_off_wps(int which)
{
	led_on(which);
}


/*start led toggle by initial a timer*/
static void wps_led_timer_func(unsigned long data) 
{

	//printk("AutoCfg_LED_Toggle_Mode :%d",AutoCfg_LED_Toggle_Mode);
	switch( AutoCfg_LED_Toggle_Mode )
	{
		case WPS_LED_INPROG_BLINK:
			if (InprogBlinkType[AutoCfg_LED_Toggle_Step])
				led_on_wps(LED_WPS_YELLOW);
			else
				led_off_wps(LED_WPS_YELLOW);

			AutoCfg_LED_Toggle_Step = (AutoCfg_LED_Toggle_Step + 1) % InprogBlinkTypeStep;

			break;
		case WPS_LED_ERROR_BLINK:
			if (ErrorBlinkType[AutoCfg_LED_Toggle_Step])
				led_on_wps(LED_WPS_RED);
			else
				led_off_wps(LED_WPS_RED);

			AutoCfg_LED_Toggle_Step = (AutoCfg_LED_Toggle_Step + 1) % ErrorBlinkTypeStep;

			break;
		case WPS_LED_OVERLAP_BLINK:
			if (OverlapBlinkType[AutoCfg_LED_Toggle_Step])
				led_on_wps(LED_WPS_RED);
			else
				led_off_wps(LED_WPS_RED);

			AutoCfg_LED_Toggle_Step = (AutoCfg_LED_Toggle_Step + 1) % OverlapBlinkTypeStep;		

			break;
		default:			
			break;
		}//switch( AutoCfg_LED_Toggle_Mode )
	if (AutoCfg_LED_Toggle_Mode != WPS_LED_STOP_BLINK)
		mod_timer(&wps_led_timer, jiffies + msecs_to_jiffies(100));
}

static void wps_led_start_toggle(void)
{
	init_timer(&wps_led_timer);
	wps_led_timer.expires = jiffies + msecs_to_jiffies(100);
	wps_led_timer.data = (unsigned long)&wps_led_timer;
	wps_led_timer.function = &wps_led_timer_func;
	mod_timer(&wps_led_timer, jiffies + msecs_to_jiffies(100));
}

static void wps_led_stop_toggle(void)
{
	del_timer(&wps_led_timer); 
}

/* WPS autoconfig initialization */
static void autoconfig_gpio_init(void)
{	
	/* stop blinking */
	AutoCfg_LED_Toggle_Mode = WPS_LED_STOP_BLINK;
	wps_led_stop_toggle();
#if defined(CONFIG_ONE_WIFI_WPS_LED)
	//WIFI and WPS at the same LED, the LED on/off should be control by WIFI driver
#else
	led_off_wps(LED_WPS_GREEN);
	led_off_wps(LED_WPS_RED);
	led_off_wps(LED_WPS_YELLOW);
#endif
}

/* WPS autoconfig off, turn off all leds */
static inline void autoconfig_gpio_off(void)
{
	autoconfig_gpio_init();
}

/* WPS autoconfig, on means green led on */
static void autoconfig_gpio_on(void)
{
	AutoCfg_LED_Toggle_Mode = WPS_LED_STOP_BLINK;
	wps_led_stop_toggle();

#if defined(CONFIG_ONE_WIFI_WPS_LED)
        //WIFI and WPS at the same LED, the LED on/off should be control by WIFI driver
#else
	led_on_wps(LED_WPS_GREEN);
#if !defined(CONFIG_ONE_WPS_LED)
	led_off_wps(LED_WPS_RED);
	led_off_wps(LED_WPS_YELLOW);
#endif
#endif
}

static void autoconfig_gpio_blink(WPS_LED_STATE_t mode)
{
	wps_led_stop_toggle();

#if defined(CONFIG_ONE_WIFI_WPS_LED)
        //WIFI and WPS at the same LED, the LED on/off should be control by WIFI driver
#else
	led_off_wps(LED_WPS_GREEN);
	led_off_wps(LED_WPS_RED);
	led_off_wps(LED_WPS_YELLOW);
#endif

	AutoCfg_LED_Toggle_Mode = mode;
	AutoCfg_LED_Toggle_Step = 0;
	wps_led_start_toggle();
}

/* WPS autoconfig, InProgress, green led blinking */
static inline void autoconfig_gpio_inprog_blink(void)
{
	autoconfig_gpio_blink(WPS_LED_INPROG_BLINK);
}

/* WPS autoconfig, Error, red led blinking */
static inline void autoconfig_gpio_error_blink(void)
{
	autoconfig_gpio_blink(WPS_LED_ERROR_BLINK);
}

/* WPS autoconfig, Overlap, red led blinking */
static inline void autoconfig_gpio_overlap_blink(void)
{
	autoconfig_gpio_blink(WPS_LED_OVERLAP_BLINK);
}
#if (defined(CONFIG_SLOT_0_8812) || defined(CONFIG_SLOT_0_8812AR_VN))  && defined(CONFIG_WLAN0_5G_WLAN1_2G)
unsigned int check_slav_on = 0;
#endif
#ifdef CONFIG_WLAN_WIFIWPSONOFF_BUTTON
extern unsigned int ext_wps_flag;
#ifdef CONFIG_APOLLO_MP_TEST
extern unsigned char led_test_start;
#endif
#endif
static int gpio_read_proc(struct seq_file *seq, void *v)
{
	char flag;
#if (defined(CONFIG_SLOT_0_8812) || defined(CONFIG_SLOT_0_8812AR_VN))  && defined(CONFIG_WLAN0_5G_WLAN1_2G)
	check_slav_on = 1;
#endif
	if(wps_wlan_restart) {
		flag = '2';
	}
#ifdef CONFIG_WLAN_WIFIWPSONOFF_BUTTON
#ifdef CONFIG_APOLLO_MP_TEST
	else if(ext_wps_flag == 1 && led_test_start == 0)
#else
	else if(ext_wps_flag == 1)
#endif
	{	
		flag = '1';
	}
	else if(ext_wps_flag == 0)
	{
		flag = '0';
	}
#else
	else if (pb_is_pushed(PB_WPS) == 1) {
		flag = '1';
	}
#endif // CONFIG_WLAN_WIFIWPSONOFF_BUTTON
	else {
		flag = '0';
	}
	seq_printf(seq,"%c\n",flag);
	
	return 0;
}


#ifdef CONFIG_RTL867X_KERNEL_MIPS16_ARCH
__NOMIPS16
#endif
static int gpio_write_proc(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
	char flag;

	if (count < 2)
		return -EFAULT;
	if (buffer && !copy_from_user(&flag, buffer, 1)) {
		if (flag == 'E'){
#if defined(CONFIG_ONE_WIFI_WPS_LED)
   			//WIFI and WPS at the same LED, the LED on/off should be control by WIFI driver
#else
			autoconfig_gpio_init();
#endif
		}
		else if (flag == '0') {
			wps_wlan_restart = 0;
			autoconfig_gpio_off();
		}
		else if (flag == '1')
			autoconfig_gpio_on();
		else if (flag == '2')
			autoconfig_gpio_inprog_blink();

#if defined (CONFIG_USB_RTL8187SU_SOFTAP) || defined (CONFIG_USB_RTL8192SU_SOFTAP) || defined(CONFIG_RTL8192CD) || defined(CONFIG_RTL8192CD_MODULE)
		else if (flag == '3')
			autoconfig_gpio_error_blink();
		else if (flag == '4')
			autoconfig_gpio_overlap_blink();
#endif //CONFIG_USB_RTL8187SU_SOFTAP
		//cathy
		else if (flag == 'R') 
			wps_wlan_restart = 1;
		return count;
	}
	else
		return -EFAULT;
}

static int ledwps_open(struct inode *inode, struct file *file)
{
        return single_open(file, gpio_read_proc, inode->i_private);
}
static const struct file_operations ledwps_fops = {
        .owner          = THIS_MODULE,
        .open           = ledwps_open,
        .read           = seq_read,
        .write          = gpio_write_proc,
        .llseek         = seq_lseek,
        .release        = single_release,
};

static int __init led_wps_init(void) {

	proc_create_data("gpio", 0644, NULL,&ledwps_fops, NULL);
	return 0;
}

static void __exit led_wps_exit(void) {
}

void wps_led_off(void){
	autoconfig_gpio_off();
}

module_init(led_wps_init);
module_exit(led_wps_exit);


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("WPS LED BEHAVIOR");

