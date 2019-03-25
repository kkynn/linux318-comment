/*
* ----------------------------------------------------------------
* Copyright c                  Realtek Semiconductor Corporation, 2003  
* All rights reserved.
* 
* Abstract: GPIO driver source code.
*
* ---------------------------------------------------------------
*/

#include <linux/kconfig.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/mman.h>
#include <linux/ioctl.h>
#include <linux/fd.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/serial_core.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/leds.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/byteorder.h>

#if defined(CONFIG_RTL8670)
#include "lx4180.h"
#elif defined(CONFIG_RTL8671)// 8671
#include "lx5280.h"
#else
//#include <platform.h>	//shlee 2.6
/*linux-2.6.19*/
#include <bspchip.h>
#endif
#include "bspgpio.h"

//#define DEBUG_GPIO
#define CONFIG_LUNA_SOC_GPIO

extern int g_internet_up;
extern int g_ppp_up;
extern unsigned int GPIO_CTRL_0, GPIO_CTRL_1, GPIO_CTRL_2, GPIO_CTRL_4;
void  gpio_led_set(void);

int led0enable=1; //for rtl8185 driver
static unsigned int GPIOdataReg[GPIO_DATA_NUM];

#ifdef CONFIG_NO_WLAN_DRIVER
#undef CONFIG_RTL8192CD
#undef CONFIG_RTL8192CD_MODULE
#endif

#if defined (CONFIG_RTL8192CD) || defined (CONFIG_RTL8192CD_MODULE)
extern void rtl8192cd_gpio_config(unsigned int wlan_idx, unsigned int gpio_num, int direction);
extern void rtl8192cd_gpio_write(unsigned int wlan_idx, unsigned int gpio_num, int value);
extern int rtl8192cd_gpio_read(unsigned int wlan_idx, unsigned int gpio_num);
#endif //CONFIG_RTL8192CD


/*
Check BD800000 to identify which GPIO pins can be used
*/
unsigned int get_GPIOMask(void)
{
	unsigned int portMask=0xFFFFFFFF;

#if  !defined(CONFIG_EXT_SWITCH)&& !defined(CONFIG_GPIO_LED_CHT_E8B)
	//portMask &= ~(GPIO_PB3|GPIO_PB4|GPIO_PB5|GPIO_PB6|GPIO_PB7);  //disable B3-B7
#endif

	return portMask;
}

/*
Config one GPIO pin. Release 1 only support output function
number and PIN location map:
Pin	num
PH7  63
:      :
PE0  32
:      :
PB7	15
:	:
PB0	8
PA7	7
:	:
PA0	0
*/
#ifdef CONFIG_LUNA_SOC_GPIO
#define SOC_GPIO_PABCD_DIR (0xb8003308)
#define SOC_GPIO_PABCD_DAT (0xb800330C)
#define SOC_GPIO_PJKMN_DIR (0xb8003348)
#define SOC_GPIO_PJKMN_DAT (0xb800334C)
#define SOC_GPIO_REG_OFFSET_BASE (0x1C)
//#define SWCORE_GPIO_EN (0xbb000048)
#define SWCORE_GPIO_EN (0xbb000038)
#endif
void gpioConfig (int gpio_num, int gpio_func)
{
	unsigned int mask;
	unsigned int reg_num = ((gpio_num>>5)<<2);
#ifdef DEBUG_GPIO  
	printk( "<<<<<<<<<enter gpioConfig(gpio_num:%d, gpio_func:%d)\n", gpio_num, gpio_func );
#endif
	if ((gpio_num >= GPIO_END)||(gpio_num < 0)) return;

#ifdef CONFIG_LUNA_SOC_GPIO
	//if (gpio_num <= GPIO_63) { /* Only 64 GPIO in SOC GPIO */
	if (gpio_num <= GPIO_95) {
		//enable
		mask = 1 << (gpio_num%32);
		REG32((SWCORE_GPIO_EN + reg_num)) |= mask;
		#ifdef DEBUG_GPIO
		printk("%s: write %x to %x\n", __FUNCTION__, REG32((SWCORE_GPIO_EN + reg_num)), (SWCORE_GPIO_EN + reg_num));
		#endif
		if(gpio_num <= GPIO_63)
		{	
			reg_num >>= 2;
			reg_num *= SOC_GPIO_REG_OFFSET_BASE;
			//select mode
			if (GPIO_FUNC_INPUT == gpio_func)
				REG32((SOC_GPIO_PABCD_DIR + reg_num)) &= ~mask;
			else
				REG32((SOC_GPIO_PABCD_DIR + reg_num)) |= mask;
			#ifdef DEBUG_GPIO
			printk("%s: write %x to %x (%d)\n", __FUNCTION__, REG32((SOC_GPIO_PABCD_DIR + reg_num)), (SOC_GPIO_PABCD_DIR + reg_num), reg_num);
			#endif	
		}else{ //GPIO64~95
			if (GPIO_FUNC_INPUT == gpio_func)
				REG32(SOC_GPIO_PJKMN_DIR) &= ~mask;
			else
				REG32(SOC_GPIO_PJKMN_DIR) |= mask;
			#ifdef DEBUG_GPIO
			printk("%s: write %x to %x\n", __FUNCTION__, REG32(SOC_GPIO_PJKMN_DIR), SOC_GPIO_PJKMN_DIR);
			#endif	
		}		
		
		//#ifdef DEBUG_GPIO
		//printk("%s: write %x to %x (%d)\n", __FUNCTION__, REG32((SOC_GPIO_PABCD_DIR + reg_num)), (SOC_GPIO_PABCD_DIR + reg_num), reg_num);
		//#endif
	}
	//else if (gpio_num <= GPIO_95) {
	//	printk("ERROR: GPIO %d (between 64~71) is not supported!!!\n", gpio_num);
	//}
#else
	if (gpio_num <= GPIO_95) {
		mask = 1 << (gpio_num%32);
		//select mode
#if 0
		if (GPIO_FUNC_INPUT == gpio_func)
			REG32((GPIO_CTRL_4 + reg_num)) &= ~mask;
		else
		       REG32((GPIO_CTRL_4 + reg_num)) |= mask;
		//enable
		REG32((GPIO_CTRL_2 + reg_num)) |= mask;
		#ifdef DEBUG_GPIO
		printk("write %x to %x\n", REG32((GPIO_CTRL_4 + reg_num)), (GPIO_CTRL_4 + reg_num));
		printk("write %x to %x\n", REG32((GPIO_CTRL_2 + reg_num)), (GPIO_CTRL_2 + reg_num));
		#endif
#endif

	}
#endif /* #ifdef CONFIG_LUNA_SOC_GPIO */
#if defined (CONFIG_RTL8192CD) || defined (CONFIG_RTL8192CD_MODULE)
	else if (gpio_num <= RTL8192CD_92E_ANTSEL_P) {
		rtl8192cd_gpio_config(WLAN_INDEX, gpio_num - RTL8192CD_GPIO_0, 
			(GPIO_FUNC_INPUT == gpio_func) ? 0x01 : 0x10);
	}
#endif //CONFIG_RTL8192CD
	else {
		printk("ERROR: GPIO %d is not supported!!!\n", gpio_num);
	}
#ifdef DEBUG_GPIO
	printk( "<<<<<<<<enter gpioConfig(gpio_num:%d, gpio_func:%d)\n", gpio_num, gpio_func );
#endif
}
EXPORT_SYMBOL(gpioConfig);

void gpioSetGIMR(int irq_num, int value)
{
#if 0
	if(value == 0)
		REG32(BSP_GIMR) &= (~(1 << irq_num));
	else
		REG32(BSP_GIMR) |= (1 << irq_num);
#endif
}


//ccwei: 120208-WPS (set gpio interrupt mask register)
void gpioSetIMR(int gpio_num, int value)
{
#if 0
	unsigned int shift;

	if ((gpio_num >= GPIO_END)||(gpio_num < 0)){
		printk("%s(%d): wrong gpio num %d\n", __FUNCTION__, __LINE__,gpio_num);
		return;
	}
	
	value &= 0x3;
	if (gpio_num <= GPIO_B_7){
		shift = ((gpio_num - GPIO_A_0)<<1);
		REG32(GPIO_PAB_IMR) &= (~(EN_BOTH_EDGE_ISR << shift));
		if (value)
			REG32(GPIO_PAB_IMR) |= (value << shift);
	}else if (gpio_num <= GPIO_D_7){
		shift = ((gpio_num - GPIO_C_0)<<1);
		REG32(GPIO_PCD_IMR) &= (~(EN_BOTH_EDGE_ISR << shift));
		if (value)
			REG32(GPIO_PCD_IMR) |= (value << shift);
	}else if (gpio_num <= GPIO_F_7){
		shift = ((gpio_num - GPIO_E_0)<<1);	
		REG32(GPIO_PEF_IMR) &= (~(EN_BOTH_EDGE_ISR << shift));
		if (value)
			REG32(GPIO_PEF_IMR) |= (value << shift);
	}else if (gpio_num <= GPIO_H_7){
		shift = ((gpio_num - GPIO_G_0)<<1);	
		REG32(GPIO_PGH_IMR) &= (~(EN_BOTH_EDGE_ISR << shift));
		if (value)
			REG32(GPIO_PGH_IMR) |= (value << shift);
	}
#endif
}

unsigned int gpioReadISR(int gpio_num)
{
#if 0
	if ((gpio_num >= GPIO_END)||(gpio_num < 0)){
		printk("%s(%d): wrong gpio num %d\n", __FUNCTION__, __LINE__,gpio_num);
		return (unsigned int)-1;
	}
	
	if (gpio_num <= GPIO_D_7) {
		return REG32(GPIO_PABCD_ISR);
	}else if (gpio_num <= GPIO_H_7){
		return REG32(GPIO_PEFGH_ISR);
	}
	
	return (unsigned int)-1;
#endif
}

void gpioClearISR(int gpio_num)
{
#if 0
	if ((gpio_num >= GPIO_END)||(gpio_num < 0)){
		printk("%s(%d): wrong gpio num %d\n", __FUNCTION__, __LINE__,gpio_num);
		return;
	}
	
	if (gpio_num <= GPIO_D_7){
		REG32(GPIO_PABCD_ISR) |= BIT(gpio_num - GPIO_A_0);
	}else if (gpio_num <= GPIO_H_7){
		REG32(GPIO_PEFGH_ISR) |= BIT(gpio_num - GPIO_E_0);
	}
#endif
}


int gpioGetBspIRQNum(int gpio_num)
{
	return BSP_GPIO1_IRQ;
}

/*set GPIO pins on*/
void gpioSet(int gpio_num)
{
	unsigned int pins;
	unsigned int reg_num = ((gpio_num>>5)<<2);
#ifdef DEBUG_GPIO 
	printk( "<<<<<<<<<enter gpioSet( gpio_num:%d )\n", gpio_num );  
#endif
	if ((gpio_num >= GPIO_END)||(gpio_num < 0)) return;

#ifdef CONFIG_LUNA_SOC_GPIO
	if (gpio_num <= GPIO_95) {
		reg_num >>= 2;
		reg_num *= SOC_GPIO_REG_OFFSET_BASE;
		pins = 1 << (gpio_num%32);
		
		if (gpio_num <= GPIO_63) {		
			//write out
			#ifdef DEBUG_GPIO
			printk( "<<<<<<<<<enter gpioSet( reg_num:%u; pins:%u )\n", reg_num, pins );  
			#endif
			REG32((SOC_GPIO_PABCD_DAT + reg_num)) |= pins;
			#ifdef DEBUG_GPIO
			printk("%s: write %x to %x\n", __FUNCTION__, REG32((SOC_GPIO_PABCD_DAT + reg_num)), (SOC_GPIO_PABCD_DAT + reg_num));
			#endif
		} else { //64~95
			#ifdef DEBUG_GPIO
			printk( "<<<<<<<<<enter gpioSet( pins:%u ) \n", pins); 
			#endif
			REG32(SOC_GPIO_PJKMN_DAT) |= pins;	
			#ifdef DEBUG_GPIO
			printk("%s: write %x to %x\n", __FUNCTION__, REG32(SOC_GPIO_PJKMN_DAT), (SOC_GPIO_PJKMN_DAT));
			#endif
		}
	} //else {
		//printk("ERROR: GPIO %d ( > 95 ) is not supported!!!\n", gpio_num);
	//}
#else
	if (gpio_num <= GPIO_95) {
		pins = 1 << (gpio_num%32);
		//write out
		REG32((GPIO_CTRL_0 + reg_num)) |= pins;
		#ifdef DEBUG_GPIO
		printk("write %x to %x\n", REG32((GPIO_CTRL_0 + reg_num)), (GPIO_CTRL_0 + reg_num));
		#endif
	}
#endif /* #ifdef CONFIG_LUNA_SOC_GPIO */
#if defined (CONFIG_RTL8192CD) || defined (CONFIG_RTL8192CD_MODULE)
	else if (gpio_num <= RTL8192CD_92E_ANTSEL_P) {
		rtl8192cd_gpio_write(WLAN_INDEX, gpio_num - RTL8192CD_GPIO_0, 1);
	}
#endif //CONFIG_RTL8192CD
	else {
		printk("ERROR: GPIO %d is not supported!!!\n", gpio_num);
	}
}
EXPORT_SYMBOL(gpioSet);

/*set GPIO pins off*/
void gpioClear(int gpio_num)
{
	unsigned int pins;
	unsigned int reg_num = ((gpio_num>>5)<<2);
	#ifdef DEBUG_GPIO
	printk( "<<<<<<<<<enter gpioClear( gpio_num:%d )\n", gpio_num );      
	#endif
	if ((gpio_num >= GPIO_END)||(gpio_num < 0)) return;

#ifdef CONFIG_LUNA_SOC_GPIO
	if (gpio_num <= GPIO_95) {
		reg_num >>= 2;
		reg_num *= SOC_GPIO_REG_OFFSET_BASE;
		pins = 1 << (gpio_num%32);
		
		if (gpio_num <= GPIO_63) {
			//write out
			REG32((SOC_GPIO_PABCD_DAT + reg_num)) &= ~pins;
			#ifdef DEBUG_GPIO
			printk("%s: write %x to %x\n", __FUNCTION__, REG32((SOC_GPIO_PABCD_DAT + reg_num)), (SOC_GPIO_PABCD_DAT + reg_num));
			#endif
		} else { //64~95
			#ifdef DEBUG_GPIO
			printk( "<<<<<<<<<enter gpioClear( pins:%u )\n", pins ); 
			#endif
			REG32(SOC_GPIO_PJKMN_DAT) &= ~pins;	
			#ifdef DEBUG_GPIO
			printk("%s: write %x to %x\n", __FUNCTION__, REG32(SOC_GPIO_PJKMN_DAT), (SOC_GPIO_PJKMN_DAT));
			#endif
		}
	}
	//else if (gpio_num <= GPIO_95) {
	//	printk("ERROR: GPIO %d (between 64~71) is not supported!!!\n", gpio_num);
	//}
#else
	if (gpio_num <= GPIO_95) {
		pins = 1 << (gpio_num%32);
	        //write out
		REG32((GPIO_CTRL_0 + reg_num)) &= ~pins;
		#ifdef DEBUG_GPIO
		printk("write %x to %x\n", REG32((GPIO_CTRL_0 + reg_num)), (GPIO_CTRL_0 + reg_num));
		#endif
	}
#endif /* #ifdef CONFIG_LUNA_SOC_GPIO */
#if defined (CONFIG_RTL8192CD) || defined (CONFIG_RTL8192CD_MODULE)
	else if (gpio_num <= RTL8192CD_92E_ANTSEL_P) {
		rtl8192cd_gpio_write(WLAN_INDEX, gpio_num - RTL8192CD_GPIO_0, 0);
	}
#endif //CONFIG_RTL8192CD
	else {
		printk("ERROR: GPIO %d is not supported!!!\n", gpio_num);
	}
}
EXPORT_SYMBOL(gpioClear);

int gpioRead(int gpio_num) 
{
	unsigned int val;
	int ret = 0;
	unsigned int reg_num = ((gpio_num>>5)<<2);
	
	if ((gpio_num >= GPIO_END)||(gpio_num < 0)) 
		return 0;

#ifdef CONFIG_LUNA_SOC_GPIO
	if (gpio_num <= GPIO_95) {
		if (gpio_num <= GPIO_63) {
			reg_num >>= 2;
			reg_num *= SOC_GPIO_REG_OFFSET_BASE;
			val = REG32(SOC_GPIO_PABCD_DAT + reg_num);
			ret = (val & (1 << (gpio_num%32))) ? 1:0;
			#ifdef DEBUG_GPIO
			printk("%s: read %x val=0x%08x, ret=%d\n", __FUNCTION__, (SOC_GPIO_PABCD_DAT + reg_num), val, ret);
			#endif
		}else { //64~95
			val = REG32(SOC_GPIO_PJKMN_DAT);
			ret = (val & (1 << (gpio_num%32))) ? 1:0;
			#ifdef DEBUG_GPIO
			printk("%s: read %x val=0x%08x, ret=%d\n", __FUNCTION__, (SOC_GPIO_PJKMN_DAT), val, ret);
			#endif	
		}
	}	
	//else if (gpio_num <= GPIO_95) {
	//	printk("ERROR: GPIO %d (between 64~71) is not supported!!!\n", gpio_num);
	//}
#else
	if (gpio_num <= GPIO_95) {
		val = REG32((GPIO_CTRL_1 + reg_num));
		ret = (val & (1 << (gpio_num%32))) ? 1:0;
	}
#endif /* #ifdef CONFIG_LUNA_SOC_GPIO */
#if defined (CONFIG_RTL8192CD) || defined (CONFIG_RTL8192CD_MODULE)
	else if (gpio_num <= RTL8192CD_92E_ANTSEL_P) {
		ret = rtl8192cd_gpio_read(WLAN_INDEX, gpio_num - RTL8192CD_GPIO_0);
	}
#endif //CONFIG_RTL8192CD
	else {
		printk("ERROR: GPIO %d is not supported!!!\n", gpio_num);
	}
	return ret;
}

// Added by Mason Yu for New map LED
void gpioHandshaking(int flag)
{
	#ifdef GPIO_LED_ADSL_HS
	gpioConfig(ADSL_LED, GPIO_FUNC_OUTPUT);		

	// on
	if ( flag == 1) {
		gpioClear(ADSL_LED);
		//gpioSet(ADSL_LED);
		return;
	}	
	
	// off
	if ( flag == 0) {
		gpioSet(ADSL_LED); 
		//gpioClear(ADSL_LED);
		return;
	}
	#elif   defined(CONFIG_GPIO_LED_CHT_E8B)
	      gpio_set_dsl_link(flag);
	#else
	return;
	#endif

}

// Added by Mason Yu for New map LED
void gpioACT(int flag)
{
	#ifdef GPIO_LED_ADSL_ACT
	gpioConfig(ADSL_ACT_LED, GPIO_FUNC_OUTPUT);
	
	// on
	if ( flag == 1) {
		gpioClear(ADSL_ACT_LED);
		return;
	}	
	
	// off
	if ( flag == 0) {
		gpioSet(ADSL_ACT_LED); 
		return;
	}
	#else
	return;
	#endif

}


// Added by Mason Yu for New map LED
void gpioAlarm(int flag)
{
	#ifdef GPIO_LED_ADSL_ALARM
	gpioConfig(ALARM_LED, GPIO_FUNC_OUTPUT);
	
	// on
	if ( flag == 1) {
		gpioClear(ALARM_LED);
		return;
	}	
	
	// off
	if ( flag == 0) {
		gpioSet(ALARM_LED); 
		return;
	}
	#else
	return;
	#endif

}

#if defined(CONFIG_STD_LED)
void gpio_LED_PPP(int flag)
{
	#ifdef GPIO_LED_PPP
	gpioConfig(PPP_LED, GPIO_FUNC_OUTPUT);
	
	// on
	if ( flag == 1) {
		gpioClear(PPP_LED);
		return;
	}	
	
	// off
	if ( flag == 0) {
		gpioSet(PPP_LED);
		return;
	}
	#else
	return;
	#endif
}
#elif defined(CONFIG_GPIO_LED_CHT_E8B)
void gpio_LED_PPP(int flag)
{
   gpio_set_ppp_g(flag);

}


#endif
void gpio_internet(int flag)
{
//	gpioAlarm(flag);

	#ifdef GPIO_LED_TR068_INTERNET
	gpioConfig(INTERNET_LED, GPIO_FUNC_OUTPUT);
	
	// on
	if ( flag == 1) {
		gpioClear(INTERNET_LED);
		return;
	}	
	
	// off
	if ( flag == 0) {
		gpioSet(INTERNET_LED);
		return;
	}
	#elif defined(CONFIG_GPIO_LED_CHT_E8B)
                  
          gpio_set_ppp_g(flag);
	
	#else
	return;
	#endif
}

#ifdef CONFIG_RTL8672_PWR_SAVE

static int lowp_proc_w(struct file *file, const char *buffer,
				unsigned long count, void *data)
{	
	char tmpbuf[100];
	int num = 0;
	//u32 saved,status;
	//u32 gimr, flags;
	extern void rtl8672_low_power(int);
#if 0
	printk("Entering SLEEP...\n");
	local_irq_save(flags);
	// mask all interrupt except for UART
	status = read_c0_status();
	gimr = REG32(GIMR);
	REG32(GIMR) = (1<<12) | (1<<13);
	write_c0_status(status & 0xFFFF0800);
	__asm__ __volatile__ (
		"sleep;"
	);
	printk("Leaving SLEEP...\n");
	write_c0_status(status);
	REG32(GIMR) = gimr;

	local_irq_restore(flags);
#endif	

	
#if 1
	if (buffer && !copy_from_user(tmpbuf, buffer, count)) {
		sscanf(tmpbuf, "%d", &num);
		rtl8672_low_power(num);					
    }
#endif	
	return count;
}

static int lowp_proc_r(char *buf, char **start, off_t offset,
			int length, int *eof, void *data)
{	
	int pos = 0;

	if (length >= 128) {
		pos += sprintf(&buf[pos], "length=%d CLK=%u\n", length, BSP_SYSCLK);
		pos += sprintf(&buf[pos], "b8002000=%02x\n", REG8(0xb8002000));
		if( IS_RLE0315 || IS_6166 ) {
			pos += sprintf(&buf[pos], "b8003200=%08x\n", REG32(0xb8003200));
			pos += sprintf(&buf[pos], "b800332C=%08x\n", REG32(0xb800332C));
		}
	}

	//pos += sprintf(&buf[pos], "%d", wifi_debug_level);
	//pos += sprintf(&buf[pos], "\n");

	return pos;
}
#endif

#if SAMPLE_CODE
static const u32 gpio_bases[3] = {	SOC_GPIO_PABCD_DIR, SOC_GPIO_PABCD_DIR+SOC_GPIO_REG_OFFSET_BASE, SOC_GPIO_PJKMN_DIR };
static int	__gpio_get_direction(struct gpio_chip *chip, unsigned offset) {
	u32 val;
	
	if (offset >= 96)
		return -EINVAL;
	
	
	val = REG32( gpio_bases[ offset >> 5 ] );
	return !!(val & (1<<(offset&0x1f)));
}

static int __gpio_get(struct gpio_chip *chip, unsigned offset) {	
	return gpioRead(offset);
}

static void	__gpio_set(struct gpio_chip *chip,unsigned offset, int value) {	
	if (value)
		gpioSet(offset);
	else
		gpioClear(offset);	
}

static int __gpio_direction_input(struct gpio_chip *chip, unsigned offset) {	
	gpioConfig(offset, GPIO_FUNC_INPUT); 	
	return 0;
}

static int __gpio_direction_output(struct gpio_chip *chip,	unsigned offset, int value) {	
	gpioConfig(offset, GPIO_FUNC_OUTPUT);
	__gpio_set(chip, offset, value);
	return 0;
}

static struct gpio_chip rtk_gpio_chip = {
	.label             = "rtk_gpio",
	.get               = __gpio_get,
	.set               = __gpio_set,
	.get_direction     = __gpio_get_direction,
	.direction_input   = __gpio_direction_input,
	.direction_output  = __gpio_direction_output,
	.base              = 0,
	.ngpio             = 64,
};

void rtk_add_led_gpio(int n_leds, struct gpio_led *p_leds) {
	struct platform_device *pdev;
    struct gpio_led_platform_data pdata;
	int err;

	pdev = platform_device_alloc("leds-gpio", -1);
	if (!pdev)
		goto err_0;

	memset(&pdata, 0, sizeof(pdata));
	pdata.num_leds = n_leds;
	pdata.leds = p_leds;
	
	err = platform_device_add_data(pdev, &pdata, sizeof(pdata));
	if (err)
		goto err_1;
	
	err = platform_device_add(pdev);
	if (err)
        goto err_1;
		
	return;
err_1:
	platform_device_put(pdev);
err_0:
	printk("%s(%d): failed\n",__func__,__LINE__);
}
#endif

int __init gpio_init(void)
{
#ifdef CONFIG_RTL8672_PWR_SAVE
	struct proc_dir_entry *proc_file1;
#endif
	int i;
	
#ifdef SAMPLE_CODE //CONFIG_GPIOLIB
	if (gpiochip_add(&rtk_gpio_chip)) {
		printk("%s(%d): failed to gpio chip\n",__func__,__LINE__);
	}
#endif

	for(i=0; i<GPIO_DATA_NUM; i++)
		GPIOdataReg[i] = 0;

#ifdef CONFIG_LUNA_SOC_GPIO
	printk("=================================\n");
	printk("CONFIG_LUNA_SOC_GPIO: %s()\n", __FUNCTION__);
	printk("=================================\n");
#if 0 /* test code */
	for(i=0; i< GPIO_64; i++) {
		if( (i == GPIO_41) || (i == GPIO_42) ) { //GPIO 41 is UART TX. GPIO 42 is UART RX
			continue;
		}
		/*
		 * GPIO #
		 * #7 LEAD_WIFI_WPS_G
		 * #10 LED_PON_G
		 * #15 LED_USB0
		 * #39 LED_PWR_G
		 */
		if( (i == GPIO_7) ||(i == GPIO_10) ||(i == GPIO_15) ||(i == GPIO_39)) {
			printk("--------------------------");
			printk("Testing GPIO %d ...\n", i);
			gpioConfig(i, GPIO_FUNC_OUTPUT);
			gpioSet(i);
			gpioRead(i);
			gpioClear(i);
		}
	}
#endif 
#endif /* #ifdef CONFIG_LUNA_SOC_GPIO */

	#ifdef CONFIG_RTL8672_PWR_SAVE
	proc_file1 = create_proc_entry("low_power",	0644, NULL);
	if(proc_file1 == NULL)
	{
		printk("can't create proc: low_power\r\n");

	} else {
		proc_file1->write_proc = lowp_proc_w;
		proc_file1->read_proc  = lowp_proc_r;
	}
	#endif
	
#if 0 //move to board-xxx.c
	unsigned int sicr;

	sicr = REG32(BSP_MISC_PINMUX);	
	if( sicr&BSP_JTAG_GPIO_PINMUX )
	{
		printk( "<<<<<<<disable GPIO JTAG function.\n" );
		REG32(BSP_MISC_PINMUX) = sicr& ~BSP_JTAG_GPIO_PINMUX;
	}
#endif

#ifdef CONFIG_GPIO_LED_CHT_E8B	
         E8GPIO_init();	 
	  gpioHandshaking(0);
         gpio_internet(0);
#endif

	return 0;
  
}


#ifdef CONFIG_GPIO_LED_CHT_E8B
// In the charge of LED blinking control timer
struct timer_list FlashLedTimer;


 
void FlashLED(unsigned long status)
{
	static int CtrlFlag = 0;

	{
	switch((unsigned char)status){
		case C_AMSW_IDLE:
			FlashLedTimer.expires=jiffies+HZ/2;
			gpioHandshaking(CtrlFlag);
			gpio_internet(0);

			break;
		case C_AMSW_L3:			
			gpioHandshaking(1);
			gpio_internet(0);
		
			break;
		case C_AMSW_ACTIVATING:
			FlashLedTimer.expires=jiffies+HZ/2;
		
			gpioHandshaking(CtrlFlag);
			gpio_internet(0);
			break;
		case C_AMSW_INITIALIZING:			
			FlashLedTimer.expires=jiffies+HZ/4;
			gpioHandshaking(CtrlFlag);
			gpio_internet(0);
		
			break;
		case C_AMSW_SHOWTIME_L0:	
			FlashLedTimer.expires=jiffies+HZ/4;

		
			// Added by Mason Yu for PPP ACT LED
//			printk("\ng_internet_up=%d\n",g_internet_up);

			if(IsTrafficOnAtm()) {		
				if(g_internet_up == 1)
					gpio_internet(CtrlFlag);
				else
					gpio_internet(0);
				#ifdef GPIO_LED_PPP_BLK
				if ( g_ppp_up >= 1)				
					gpio_LED_PPP(!CtrlFlag);
				#endif
			}			
			else {	
				if(g_internet_up == 1)
				{
					CtrlFlag = 1;
					gpio_internet(CtrlFlag);
				
				}
				else
					gpio_internet(0);
				#ifdef GPIO_LED_PPP_BLK
				if ( g_ppp_up >= 1)				
					gpio_LED_PPP(1);
				#endif
			}		
			 gpioHandshaking(1);
                      break;
	   
		case C_AMSW_END_OF_LD:
			
			gpioHandshaking(0);	
			gpio_internet(0);
			break;
	}

	if(CtrlFlag)
		CtrlFlag = 0;
	else
		CtrlFlag = 1;	

	FlashLedTimer.data = status;	
	FlashLedTimer.function=(void (*)(unsigned long))FlashLED;
	mod_timer(&FlashLedTimer, FlashLedTimer.expires);	

	}
	
}	



/*
 * ADSL_state : Called by DSP driver while ADSL status changing
 */

void ADSL_state(unsigned char LEDstate)
{
	static unsigned char LastStatus = 255;

	if(LastStatus != LEDstate){
		LastStatus = LEDstate;
		FlashLED((unsigned long)LEDstate);
	}
}	
#endif

//if 128 PIN board 
//PA5 PA6 PA7 PB0 PB1 PB2 PB3 PB4 PB5 PB6 PB7
//0        1    2     3     4     5    6      7      8    9    10
//74164 used

//if 208 PIN board
#define LED_CLK    14    //GPIO_B_6
#define LED_DATA  15    //GPIO_B_7


#define POWER_GREEN            0
#define POWER_RED                1
#define PPP_GREEN                  2
#define PPP_RED                      3               
#define LED_DSL_LINK             4 
/* moved to gpio.h
#define LED_WPS_G                 5
#define LED_WPS_R                 6
#define LED_WPS_Y                 7
*/
static int   power_red_on    ;
static int   power_green_on   ;
static int   ppp_red_on ;
static int   ppp_green_on ;
static int   led_dsl_link_on  ;
static int   led_wps_g_on  = 0 ;
static int   led_wps_r_on = 0;
static int   led_wps_y_on = 0;

void gpio_set_power_g(int flag)
{
    if(flag) //on  
       power_green_on = 1;
    else 
       power_green_on = 0;		
    gpio_led_set();
}

void gpio_set_power_r(int flag)
{
    if(flag)
       power_red_on = 1;
    else
       power_red_on = 0;		 
    gpio_led_set();
}

void gpio_set_ppp_g(int flag)
{                
     if(flag)
	 ppp_green_on = 1;
     else
	 ppp_green_on = 0;
     gpio_led_set();	 
     	 
}

void gpio_set_ppp_r(int flag)
{
     if(flag)
	 ppp_red_on = 1;
     else
	 ppp_red_on = 0;
     gpio_led_set();	 
     	 
}

void gpio_set_dsl_link(int flag)
{
     if(flag)
	led_dsl_link_on  = 1;
     else
	 led_dsl_link_on = 0;
     gpio_led_set();	 
     	 
}
void  gpio_led_set(void) {
	gpioConfig(LED_DATA, GPIO_FUNC_OUTPUT);
	gpioConfig(LED_CLK, GPIO_FUNC_OUTPUT);

   
       if(led_wps_y_on){
	 	gpioClear(LED_DATA);	
       }
	else {
		gpioSet(LED_DATA);
	}
	
	gpioClear(LED_CLK);  
	gpioSet(LED_CLK); 	   //clock 1
	if(led_wps_r_on)
	{
          gpioClear(LED_DATA);
	}
	else 
	{
		gpioSet(LED_DATA);
	}
	gpioClear(LED_CLK)	;
	gpioSet(LED_CLK);     //clock  2  

	if(led_wps_g_on){
            gpioClear(LED_DATA);
	}	
	else {
         gpioSet(LED_DATA);       	
	}

	gpioClear(LED_CLK)	;  
	gpioSet(LED_CLK);     //clock  3
	if(ppp_red_on)
	{
             gpioClear(LED_DATA);

	}
	else
	{
             gpioSet(LED_DATA);

	}
	gpioClear(LED_CLK)	;
	gpioSet(LED_CLK);     //clock  4
	if(ppp_green_on){
         gpioClear(LED_DATA);   
	}
	else
	{
            gpioSet(LED_DATA);

	}
	gpioClear(LED_CLK)	;  
	gpioSet(LED_CLK);     //clock   5 
	if(led_dsl_link_on)
	{
           gpioClear(LED_DATA);

	}
	else 
	{
	  gpioSet(LED_DATA);
	}
	  gpioClear(LED_CLK)	;
	  gpioSet(LED_CLK);     //clock   6
	if(power_red_on)
	{
	  gpioClear(LED_DATA);
	}
	else
	{
	    
	   gpioSet(LED_DATA);
	}
	gpioClear(LED_CLK)	;
	gpioSet(LED_CLK);     //clock    7
	if(power_green_on)
	{
              gpioClear(LED_DATA);
	
	}
	else
	{
             gpioSet(LED_DATA);
	}
	gpioClear(LED_CLK)	;
	gpioSet(LED_CLK);     //clock     8 
     


}
 #ifdef CONFIG_GPIO_LED_CHT_E8B 
void E8GPIO_init()
{

	init_timer(&FlashLedTimer);	

}
#endif

static void __exit gpio_exit (void)
{
}

module_init(gpio_init);
module_exit(gpio_exit);

