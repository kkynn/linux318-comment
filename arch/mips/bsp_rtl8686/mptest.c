#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include "led-generic.h"
#include "pushbutton.h"
#include "bspgpio.h"
#include "bspchip.h"

#include <rtk/led.h>
#include <rtk/init.h>
#include <rtk/i2c.h>
//CONFIG_GPON_VERSION is define in 
//linux-2.6.x/drivers/net/rtl86900/Compiler_Flag
//#define CONFIG_GPON_VERSION 2
//#if CONFIG_GPON_VERSION < 2
//#include <rtk/gpon.h>
//#else
#include <module/gpon/gpon.h>
//#endif


//#define CONFIG_RTL8696_SERIES
//#define CONFIG_RTL9607_SERIES
//#define CONFIG_RTL9602_SERIES
//#define CONFIG_RTL9607P_SERIES


#define DEBUG
#ifdef DEBUG
#define PRINT printk
#else
#define PRINT
#endif

#ifdef CONFIG_APOLLO_MP_TEST
static struct proc_dir_entry *led_test=NULL;
unsigned char led_test_start=0;
unsigned char usbtest=0, ethtest=0;
extern char default_flag;
extern char reboot_flag;
extern char wlan_onoff_flag;
#endif


#define MP_LOW_ACTIVE	1
#if MP_LOW_ACTIVE

#define MP_GPIO_SET(w, op)  do { \
	gpioConfig(w, GPIO_FUNC_OUTPUT); \
	if (LED_ON==op) gpioClear(w); \
	else gpioSet(w); \
} while (0);

#define MP_GPIO_READ(w) (!gpioRead(w))
		
#else

#define MP_GPIO_SET(w, op)  do { \
	gpioConfig(w, GPIO_FUNC_OUTPUT); \
	if (LED_ON==op) gpioSet(w); \
	else gpioClear(w); \
} while (0);

#define MP_GPIO_READ(w) (gpioRead(w))
#endif

void record_gpio_state(void)
{
	#ifdef CONFIG_RTL9607C_SERIES
	GPIO_EN_0 = REG32(IO_GPIO_EN_REG);
	GPIO_EN_1 = REG32(IO_GPIO_EN_REG+4);
	GPIO_EN_2 = REG32(IO_GPIO_EN_REG+8);
	PRINT("[%s-%d]GPIO_EN_0=0x%x, GPIO_EN_1=0x%x, GPIO_EN_2=0x%x\n",__func__,__LINE__,GPIO_EN_0,GPIO_EN_1,GPIO_EN_2);
	#endif
}
void restore_gpio_state(void)
{
	#ifdef CONFIG_RTL9607C_SERIES
	PRINT("[%s-%d]GPIO_EN_0=0x%x, GPIO_EN_1=0x%x, GPIO_EN_2=0x%x\n",__func__,__LINE__,GPIO_EN_0,GPIO_EN_1,GPIO_EN_2);
	REG32(IO_GPIO_EN_REG) = GPIO_EN_0;
	REG32(IO_GPIO_EN_REG+4) = GPIO_EN_1;
	REG32(IO_GPIO_EN_REG+8) = GPIO_EN_2;
	#endif
}


typedef enum MP_LED_TYPE
{
	LED_LAN0 = 0,
	LED_LAN1,
	LED_LAN2,
	LED_LAN3,
	LED_LAN4, // 8696/9607P
	LED_LAN5, // 8696/9607P
	LED_PON,
	LED_LOS, // 9601b
	LED_USB0,
	LED_USB1,
	LED_WPSR,
	LED_WPSY,
	LED_WPSG,
	LED_NET_G,
	LED_NET_R,	
	LED_WAN, // 9607
	LED_1G, // 9601b
	LED_SATA, //9607P
	LED_PWR, //9607P
	LED_FX0,
	LED_FX1,
	LED_END
	// wlan0/wlan1/fxs0/fxs1 control by userspace 
}mp_led_type;


struct led_gpio_struct
{
	mp_led_type mp_led_type;
	unsigned int mp_led_gpio_index;
	unsigned int mp_led_gpio_state;
}
#if defined(CONFIG_RTL8696_SERIES)
RTL8696[]={{LED_WAN,58,0},{LED_USB0,21,0},{LED_USB1,22,0},{LED_LAN0,13,1},
			{LED_LAN1,24,1},{LED_LAN2,46,0},{LED_LAN3,48,0},{LED_NET_G,35,0}};
#elif defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9607)
RTL9607[]={{LED_WAN,21,0},{LED_USB0,48,0},{LED_USB1,24,1},{LED_LAN0,58,0},
			{LED_LAN1,59,0},{LED_LAN2,46,0},{LED_LAN3,13,0},{LED_NET_G,60,0},
			{LED_PON,22,0}};
#elif defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9607_IAD_V00)
RTL9607[]={{LED_LAN0,13,0},{LED_LAN1,24,1},{LED_LAN2,46,0},{LED_LAN3,48,0},
			{LED_USB0,21,0},{LED_NET_G,23,0},{LED_PON,58,0},{LED_LOS,47,0},
			{LED_PWR,10,0},{LED_FX0,31,0}, {LED_FX1,1,0}};
#elif defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9607P)
RTL9607P[]={{LED_LAN4,47,0},{LED_USB0,56,0},{LED_USB1,55,0},{LED_LAN0,13,1},
			{LED_LAN1,24,1},{LED_LAN2,46,0},{LED_LAN3,48,0},{LED_NET_G,23,0},
			{LED_PON,58,0},{LED_SATA,57,0}};
#elif defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9602B)
RTL9602[]={{LED_LAN1,13,0},{LED_LAN2,24,1},{LED_NET_G,45,0},{LED_PON,22,0}};
#elif defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9602C)
RTL9602[]={{LED_LAN0,26,0},{LED_LAN1,39,0},{LED_NET_G,31,0},{LED_NET_R,35,0},{LED_PWR,36,0},
                         {LED_USB0,27,0}, {LED_PON,29,0},{LED_LOS,28,0}};
                        // {LED_USB0,27,0}, {LED_PON,29,0}, {LED_LOS,28,0}};
#else
;
#endif
//PON_LED_PROFILE_DEMO_RTL9601
//PON_LED_PROFILE_DEMO_RTL9601B

struct ledstruct
{
	mp_led_type mp_led_type;
	unsigned int mp_led_index;
	unsigned int mp_led_state;
	rtk_led_type_t rtk_led_type;
}
RTL9601b[]={{LED_LAN0,0,0,LED_TYPE_UTP0},{LED_PON,1,0,LED_TYPE_PON},{LED_LOS,2,0,LED_TYPE_PON},{LED_1G,3,0,LED_TYPE_UTP0}};

/*global variable for led/push button test!*/
unsigned int gMPTEST_STATE = 0;

static void print_string(char *str)
{
	printk("%s", str);
}

unsigned int mptest_state(void)
{
	return gMPTEST_STATE;
}
		
void gpioDisable(int gpio_num)
{
        unsigned int reg_num = ((gpio_num>>5)<<2);
        unsigned int mask;

        if ((gpio_num >= 72)||(gpio_num < 0)) return;
        if (gpio_num <= 71)
        {
                mask = 1 << (gpio_num%32);
                REG32((0xBB0000F0 + reg_num)) &= ~mask;
        }
}

char wps_led_restore_state;
#ifndef CONFIG_PON_LED_PROFILE_DEMO_RTL9601B
static char led_restore_state[LED_END];

static int __save_led_state(struct led_gpio_struct *led_struct,int led_state)
{
	//printk("in <%s>save led\n",__func__);
	if(led_struct==NULL || led_struct->mp_led_type<0 || led_struct->mp_led_type>=LED_END )
		return -1;
	if((led_struct->mp_led_type >= LED_LAN0) && (led_struct->mp_led_type <= LED_PON))//lan led control by switch directly
		return 0;
	//printk("save led:%d  state:%d\n",led_struct->mp_led_type,led_state);
	led_restore_state[led_struct->mp_led_type] = led_state;
	return 0;
}

static int save_led_state(struct led_gpio_struct *led,int len)
{
	int i;
	for(i=0; i<len; i++)
	{
		__save_led_state(&led[i], MP_GPIO_READ(led[i].mp_led_gpio_index)?LED_ON:LED_OFF);
	}
	return 0;
}

int update_led_state(mp_led_type led_type, int led_state)
{
	int i,len=0;
	struct led_gpio_struct* led=NULL;
	//printk("in <%s %d> update led state\n",__func__,__LINE__);
	//if(gMPTEST_STATE == 0)
		//return 0;
	if(led_type<LED_LAN0 || led_type>=LED_END)
		return -1;
	if(led_state!=LED_ON && led_state!=LED_OFF)
		return -1;		
#if defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9607_IAD_V00)||defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9607_IAD_V01)
	led=RTL9607;
	len=sizeof(RTL9607)/sizeof(RTL9607[0]);
#elif defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9602C)
	led=RTL9602;
	len=sizeof(RTL9602)/sizeof(RTL9602[0]);
#endif
	if(led==NULL || len==0)
		return -1;
	//printk("in <%s %d>\n",__func__,__LINE__);
	for(i=0; i<len; i++)
	{
		if(led[i].mp_led_type == led_type)
		{
			//printk("in <%s> led:%d state:%d\n",__func__,led_type,led_state);
			__save_led_state(&(led[i]), led_state);
			break;
		}
	}
	return 0;
}

static int restore_led_state(struct led_gpio_struct *led_struct,int len)
{
	int i,j;
	if(led_struct==NULL)
		return -1;
	//printk("in <%s>restore led\n",__func__);
	for(i=0; i<LED_END; i++)
	{
		for(j=0;j<len;j++)
		{
			if(led_struct[j].mp_led_type != i)
				continue;
			break;
		}
		if(j==len)
			continue;
		if(led_restore_state[i]!=LED_ON && led_restore_state[i]!=LED_OFF)
			continue;
		//printk("led %d state %d\n",led_struct[j].mp_led_type,led_restore_state[i]);
		MP_GPIO_SET(led_struct[j].mp_led_gpio_index, led_restore_state[i]);
	}
	return 0;
}
#endif

#ifdef CONFIG_PON_LED_PROFILE_DEMO_RTL9601B			
void mptest_off(struct ledstruct led[],unsigned int total_leds)
{
	int i = 0;
	rtk_led_config_t ledConfig;

	memset(&ledConfig,0x0,sizeof(rtk_led_config_t));
	ledConfig.ledEnable[LED_CONFIG_FORCE_MODE] = ENABLED;
	
	for(i=0;i<total_leds;i++)
	{
		rtk_led_parallelEnable_set (led[i].mp_led_index,ENABLED);
		rtk_led_config_set(led[i].mp_led_index,led[i].rtk_led_type,&ledConfig);
		rtk_led_modeForce_set(led[i].mp_led_index,LED_FORCE_OFF);
	}
	
	gMPTEST_STATE = 0;
}
#else
void mptest_off(struct led_gpio_struct led[],unsigned int total_leds)
{
	int i = 0;
	int OP;
	for(i=0;i<total_leds;i++)
	{
		if(led[i].mp_led_gpio_state) 
			OP = LED_ON;//high active
		else
			OP = LED_OFF;//low active	
		MP_GPIO_SET(led[i].mp_led_gpio_index, led[i].mp_led_gpio_state);
	}
}
#endif

#ifdef CONFIG_PON_LED_PROFILE_DEMO_RTL9601B
void mptest_on(struct ledstruct led[], unsigned int total_leds)
{
	int i = 0;
	rtk_led_config_t ledConfig;
	gMPTEST_STATE = 1;
	memset(&ledConfig,0x0,sizeof(rtk_led_config_t));
	ledConfig.ledEnable[LED_CONFIG_FORCE_MODE] = ENABLED;
	for(i=0;i<total_leds;i++)
	{
		rtk_led_parallelEnable_set (led[i].mp_led_index,ENABLED);
		rtk_led_config_set(led[i].mp_led_index,led[i].rtk_led_type,&ledConfig);
		rtk_led_modeForce_set(led[i].mp_led_index,LED_FORCE_ON);
	}
}
#else
void pwrled_on(struct led_gpio_struct led[], unsigned int total_leds)
{
                int i = 0;
        int OP=0;
        gMPTEST_STATE = 1;
        PRINT("%s-%d total_leds=%d\n",__func__,__LINE__,total_leds);
        for(i=0;i<total_leds;i++)
        {
                if(led[i].mp_led_type == LED_PWR)
                {
                        MP_GPIO_SET(led[i].mp_led_gpio_index, LED_ON);
                }
        }

}

void mptest_on(struct led_gpio_struct led[], unsigned int total_leds)
{
	int i = 0;
	int OP=0;
	gMPTEST_STATE = 1;
	PRINT("%s-%d total_leds=%d\n",__func__,__LINE__,total_leds);
	for(i=0;i<total_leds;i++)
	{
		if(led[i].mp_led_gpio_state) 
			OP = LED_OFF;//high active
		else
			OP = LED_ON;//low active
		PRINT("%s-%d  [%d].index=%d [%d].state=%d\n",__func__,__LINE__,i,led[i].mp_led_gpio_index,i,led[i].mp_led_gpio_state);
		MP_GPIO_SET(led[i].mp_led_gpio_index, OP);
	}
}
#endif

static void wps_8812_led_on(void)
{
	PRINT("WPS 8812 ON\n");
	REG32(0xBA000044) &= ~(1<<12) ;
	REG32(0xBA000044) |= (1<<20);
}

static void wps_8812_led_off(void)
{
	PRINT("WPS 8812 OFF\n");
	REG32(0xBA000044) |= (1<<12)|(1<<20) ;
	REG32(0xBA00004C) &= ~(1 <<23);
}

static void wps_8192_led_on(void)
{
#if defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9601B)
#elif defined(CONFIG_RTL8696_SERIES)
	PRINT("WPS 8192 ON\n");
	MP_GPIO_SET(RTL8192CD_GPIO_4, LED_ON);
#elif defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9607)
	PRINT("WPS 8192 ON\n");
	MP_GPIO_SET(RTL8192CD_GPIO_4, LED_ON); //wps_G
	MP_GPIO_SET(RTL8192CD_GPIO_0, LED_ON); //wps_R
	MP_GPIO_SET(RTL8192CD_92E_ANTSEL_P, LED_ON); //wps_Y
#elif defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9607_IAD_V00)
	PRINT("WPS 8192 ON\n");
	MP_GPIO_SET(RTL8192CD_GPIO_4, LED_ON); //wps_G
	MP_GPIO_SET(RTL8192CD_GPIO_0, LED_ON); //wps_R
	MP_GPIO_SET(RTL8192CD_92E_ANTSEL_P, LED_ON); //wps_Y
#elif defined(CONFIG_CONFIG_PON_LED_PROFILE_DEMO_RTL9607P)
	PRINT("WPS 8192 ON\n");
	/*Dual linux can't find CONFIG_SLOT_1_8192EE in master*/
	//WPS_G on
	PRINT("WPS ON 92E\n");
	REG32(0xB9000044) &= ~(1<<12);
	REG32(0xB9000044) |= (1<<20);
#elif defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9602B)
#elif defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9602C)
        PRINT("WPS 8192 ON\n");
        MP_GPIO_SET(RTL8192CD_GPIO_4, LED_ON); //wps_G
        MP_GPIO_SET(RTL8192CD_GPIO_0, LED_ON); //wps_R
        MP_GPIO_SET(RTL8192CD_92E_ANTSEL_P, LED_ON); //wps_Y
#endif
}

static void wps_8192_led_off(void)
{

#if defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9601B)
#elif defined(CONFIG_RTL8696_SERIES)
	PRINT("WPS 8192 OFF\n");
	MP_GPIO_SET(RTL8192CD_GPIO_4, LED_OFF);
#elif defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9607)
	PRINT("WPS 8192 OFF\n");
	MP_GPIO_SET(RTL8192CD_GPIO_4, LED_OFF); //wps_G
	MP_GPIO_SET(RTL8192CD_GPIO_0, LED_OFF); //wps_R
	MP_GPIO_SET(RTL8192CD_92E_ANTSEL_P, LED_OFF); //wps_Y	
#elif defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9607_IAD_V00)
	PRINT("WPS 8192 OFF\n");
	MP_GPIO_SET(RTL8192CD_GPIO_4, LED_OFF); //wps_G
	MP_GPIO_SET(RTL8192CD_GPIO_0, LED_OFF); //wps_R
	MP_GPIO_SET(RTL8192CD_92E_ANTSEL_P, LED_OFF); //wps_Y	
#elif defined(CONFIG_CONFIG_PON_LED_PROFILE_DEMO_RTL9607P)
	//WPS_G off
	PRINT("WPS 8192 OFF\n");
	REG32(0xB9000044) |= (1<<12)|(1<<20) ;
	REG32(0xB900004C) &= ~(1 <<23);
#elif defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9602B)
#elif defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9602C)
        PRINT("WPS 8192 OFF\n");
        MP_GPIO_SET(RTL8192CD_GPIO_4, LED_OFF); //wps_G
        MP_GPIO_SET(RTL8192CD_GPIO_0, LED_OFF); //wps_R
        MP_GPIO_SET(RTL8192CD_92E_ANTSEL_P, LED_OFF); //wps_Y
#endif

}

static int mptest_read_proc(char *page, char **start, off_t off,
				int count, int *eof, void *data)
{
	int len = 0;
	return len;
}

static int mptest_level_write(struct file *file, const char __user *buffer, size_t count, loff_t *ppos)
{
	char flag;
	static unsigned int regValue = 0;	
	static unsigned int gpio_restore_value_48, gpio_restore_value_4c;
	static char led_mp_test_flag=0;//flag for store led status
	if (buffer && !copy_from_user(&flag, buffer, 1)) 
	{
#ifdef CONFIG_APOLLO_MP_TEST
		if(led_test_start)
			return count;
#endif
		if((flag == '0' || flag == '1') && (led_mp_test_flag == 0) )//store led state for restore
		{
			int i,len=0;
			struct led_gpio_struct* led=NULL;
#if defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9607_IAD_V00)||defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9607_IAD_V01)
			led=RTL9607;
			len=sizeof(RTL9607)/sizeof(RTL9607[0]);
#elif defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9602C)
			led=RTL9602;
			len=sizeof(RTL9602)/sizeof(RTL9602[0]);
#elif defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9607C_TBL_B)			
			led=RTL9607CP;
			len=sizeof(RTL9607CP)/sizeof(RTL9607CP[0]);
			if(0 != REG32(IO_LED_EN_REG))
				regValue = REG32(IO_LED_EN_REG); //record current status
			PRINT("[%s-%d] led_mp_test_flag=%d regValue=0x%08X IO_LED_EN_REG=0x%08X\n",__func__,__LINE__,led_mp_test_flag,regValue, IO_LED_EN_REG);
			REG32(IO_LED_EN_REG) = 0;
			record_gpio_state();
#endif
			if(led!=NULL && len!=0)
				save_led_state(led, len);

#if defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9602C)
			if(0x3000 != REG32(0xBB023014))
				regValue = REG32(0xBB023014); //record current status
			REG32(0xBB023014) =  0x3000;//pon and los led don't set gpio mode
			if(0 != REG32(0xBB000048))
                gpio_restore_value_48 = REG32(0xBB000048);
			if(0 != REG32(0xBB00004C))
				gpio_restore_value_4c = REG32(0xBB00004C);
#elif defined(CONFIG_RTL8696_SERIES) || defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9607)|| defined(CONFIG_CONFIG_PON_LED_PROFILE_DEMO_RTL9607P)	|| defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9607_IAD_V00) || defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9607_IAD_V01)||defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9602B)	
			if(0 != REG32(0xBB023014))
				regValue = REG32(0xBB023014); //record current status
			REG32(0xBB023014) = 0;
#endif
			led_mp_test_flag=1;
		}
		
		if (flag == '0')
		{
#if defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9601B)
			print_string("LED OFF\n");
			mptest_off(RTL9601b,sizeof(RTL9601b)/sizeof(RTL9601b[0]));
#elif defined(CONFIG_RTL8696_SERIES)
			print_string("LED OFF\n");
			mptest_off(RTL8696,sizeof(RTL8696)/sizeof(RTL8696[0]));
	#if (defined(CONFIG_SLOT_0_8812) || defined(CONFIG_SLOT_0_8812AR_VN))  && defined(CONFIG_WLAN0_5G_WLAN1_2G)
			wps_8812_led_off();
	#elif (defined(CONFIG_SLOT_0_8192EE) || defined(CONFIG_SLOT_1_8192EE) )
			wps_8192_led_off();
	#endif
#elif defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9607)
			print_string("LED OFF\n");
			mptest_off(RTL9607,sizeof(RTL9607)/sizeof(RTL9607[0]));
	#if (defined(CONFIG_SLOT_0_8812) || defined(CONFIG_SLOT_0_8812AR_VN))  && defined(CONFIG_WLAN0_5G_WLAN1_2G)
			wps_8812_led_off();
	#elif (defined(CONFIG_SLOT_0_8192EE) || defined(CONFIG_SLOT_1_8192EE) )
			wps_8192_led_off();
	#endif	
#elif defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9607_IAD_V00)
			print_string("RTL9607 LED OFF\n");
			mptest_off(RTL9607,sizeof(RTL9607)/sizeof(RTL9607[0]));
	#if (defined(CONFIG_SLOT_0_8812) || defined(CONFIG_SLOT_0_8812AR_VN))  && defined(CONFIG_WLAN0_5G_WLAN1_2G)
			wps_8812_led_off();
	#elif (defined(CONFIG_SLOT_0_8192EE) || defined(CONFIG_SLOT_1_8192EE) )
			wps_8192_led_off();
	#endif		
#elif defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9602B)	
			print_string("LED OFF\n");
			mptest_off(RTL9602,sizeof(RTL9602)/sizeof(RTL9602[0]));
#elif defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9602C)	
			print_string("RTL9602 LED OFF\n");
			mptest_off(RTL9602,sizeof(RTL9602)/sizeof(RTL9602[0]));
	#if (defined(CONFIG_SLOT_0_8812) || defined(CONFIG_SLOT_0_8812AR_VN))  && defined(CONFIG_WLAN0_5G_WLAN1_2G)
			wps_8812_led_off();
	#elif (defined(CONFIG_SLOT_0_8192EE) || defined(CONFIG_SLOT_1_8192EE) )
			wps_8192_led_off();
	#endif
#elif defined(CONFIG_CONFIG_PON_LED_PROFILE_DEMO_RTL9607P)
			print_string("LED OFF\n");
			mptest_off(RTL9607P,sizeof(RTL9607P)/sizeof(RTL9607P[0]));
	#if (defined(CONFIG_SLOT_0_8812) || defined(CONFIG_SLOT_0_8812AR_VN))  && defined(CONFIG_WLAN0_5G_WLAN1_2G)
			wps_8812_led_off();
	#elif (defined(CONFIG_SLOT_0_8192EE) || defined(CONFIG_SLOT_1_8192EE) )
			wps_8192_led_off();
	#endif		
#endif
		}
		else if (flag == '1')
		{
#if defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9601B)
			PRINT("LED ON\n");
			mptest_on(RTL9601b,sizeof(RTL9601b)/sizeof(RTL9601b[0]));
#elif defined(CONFIG_RTL8696_SERIES)
			PRINT("RTL8696 LED ON\n");
			regValue = REG32(0xBB023014); //record current status
			REG32(0xBB023014) = 0;
			mptest_on(RTL8696,sizeof(RTL8696)/sizeof(RTL8696[0]));
	#if (defined(CONFIG_SLOT_0_8812) || defined(CONFIG_SLOT_0_8812AR_VN))  && defined(CONFIG_WLAN0_5G_WLAN1_2G)
			wps_8812_led_on();
	#elif (defined(CONFIG_SLOT_0_8192EE) || defined(CONFIG_SLOT_1_8192EE))
			wps_8192_led_on();
	#endif
#elif defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9607)
			PRINT("RTL9607 LED ON\n");
			regValue = REG32(0xBB023014); //record current status
			REG32(0xBB023014) = 0;
			mptest_on(RTL9607,sizeof(RTL9607)/sizeof(RTL9607[0]));
	#if (defined(CONFIG_SLOT_0_8812) || defined(CONFIG_SLOT_0_8812AR_VN))  && defined(CONFIG_WLAN0_5G_WLAN1_2G)
			wps_8812_led_on();
	#elif (defined(CONFIG_SLOT_0_8192EE) || defined(CONFIG_SLOT_1_8192EE))
			wps_8192_led_on();
	#endif	
#elif defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9607_IAD_V00)
			PRINT("RTL9607 LED ON\n");
			if(0 != REG32(0xBB023014))
				regValue = REG32(0xBB023014); //record current status
			REG32(0xBB023014) = 0;
			mptest_on(RTL9607,sizeof(RTL9607)/sizeof(RTL9607[0]));
	#if (defined(CONFIG_SLOT_0_8812) || defined(CONFIG_SLOT_0_8812AR_VN))  && defined(CONFIG_WLAN0_5G_WLAN1_2G)
			wps_8812_led_on();
	#elif (defined(CONFIG_SLOT_0_8192EE) || defined(CONFIG_SLOT_1_8192EE))
			wps_8192_led_on();
	#endif	
#elif defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9602B)	
			PRINT("RTL9602 LED ON\n");
			regValue = REG32(0xBB023014); //record current status
			REG32(0xBB023014) = 0;
			mptest_on(RTL9602,sizeof(RTL9602)/sizeof(RTL9602[0]));
#elif defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9602C)
			PRINT("RTL9602 LED ON\n");
			//if(0 != REG32(0xBB023014))
			//	regValue = REG32(0xBB023014); //record current status
			//REG32(0xBB023014) = 0;
			mptest_on(RTL9602,sizeof(RTL9602)/sizeof(RTL9602[0]));
	#if (defined(CONFIG_SLOT_0_8812) || defined(CONFIG_SLOT_0_8812AR_VN))  && defined(CONFIG_WLAN0_5G_WLAN1_2G)
			wps_8812_led_on();
	#elif (defined(CONFIG_SLOT_0_8192EE) || defined(CONFIG_SLOT_1_8192EE))
			wps_8192_led_on();
	#endif	
#elif defined(CONFIG_CONFIG_PON_LED_PROFILE_DEMO_RTL9607P)	
			PRINT("RTL9607P LED ON\n");
			regValue = REG32(0xBB023014); //record current status
			REG32(0xBB023014) = 0;
			mptest_on(RTL9607P,sizeof(RTL9607P)/sizeof(RTL9607P[0]));
	#if ((defined(CONFIG_SLOT_0_8812) || defined(CONFIG_SLOT_0_8812AR_VN)) && defined(CONFIG_WLAN0_5G_WLAN1_2G))
			wps_8812_led_on();		
	#else
			wps_8192_led_on();
	#endif	
#endif
		}else if (flag == '2'){
			#if defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9607_IAD_V00)
			pwrled_on(RTL9607,sizeof(RTL9607)/sizeof(RTL9607[0]));
			#endif
			//restore status
			if(regValue != 0)
				REG32(0xBB023014) = regValue;
			gMPTEST_STATE = 0;
		}
		
		return count;
	}
	return -EFAULT;
}



#ifdef CONFIG_APOLLO_MP_TEST
#ifdef CONFIG_PON_LED_PROFILE_DEMO_RTL9601B
#define P_ABLTY 0xBB0000EC
#define PORT_NUM 1
#else
#ifdef CONFIG_RTL9602C_SERIES
#define P_ABLTY 0xBB000198
#define PORT_NUM 2
#else
#define P_ABLTY 0xBB0000BC
#define PORT_NUM 4
#endif
#endif
static int led_test_read(struct seq_file *seq, void *v)
{	  
	int len=0;

	if (ethtest) {
		int LinkStatus, portnum;
		rtk_gpon_fsm_status_t state;
	
		for (portnum=0; portnum<PORT_NUM; portnum++) {
			LinkStatus = ((*(volatile unsigned int *)(P_ABLTY + (portnum<<2))) >> 4)&0x1;
			seq_printf(seq, "Test Switch LAN PORT %d %s\n",
					portnum+1, LinkStatus?"UP":"DOWN");
		}
		rtk_gponapp_ponStatus_get(&state);
		if(state == GPON_STATE_O5)
			LinkStatus = 1;
		else
			LinkStatus = 0;
		seq_printf(seq, "Test Switch PON PORT %s\n", LinkStatus?"UP":"DOWN");
		ethtest = 0;
	}
	else if (led_test_start) {
		if ('0' != default_flag)
			seq_printf(seq, "RESET success!\n");
		/*FIXME. where should I get the wps button info? */
		if (pb_is_pushed(PB_WPS) == 1)
			seq_printf(seq, "WPS success!\n");
#ifdef CONFIG_WLAN_ON_OFF_BUTTON		
		if ('0' != wlan_onoff_flag)
			seq_printf(seq, "WLAN success!\n");
#endif		
	}

	return len;
}

static int led_set_single_light(mp_led_type mp_led_type)
{
	int i;
	rtk_led_config_t ledConfig;
	
#if defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9601B)
	int total_leds = (sizeof(RTL9601b)/sizeof(RTL9601b[0]));
	memset(&ledConfig,0x0,sizeof(rtk_led_config_t));
	ledConfig.ledEnable[LED_CONFIG_FORCE_MODE] = ENABLED;
	//printk("total_leds = %d\n",total_leds);

	for(i=0;i<total_leds;i++)
	{
		if(RTL9601b[i].mp_led_type == mp_led_type){
			rtk_led_parallelEnable_set (RTL9601b[i].mp_led_index,ENABLED);
			rtk_led_config_set(RTL9601b[i].mp_led_index,RTL9601b[i].rtk_led_type,&ledConfig);
			rtk_led_modeForce_set(RTL9601b[i].mp_led_index,LED_FORCE_ON);
			return 0;
		}
	}
#elif defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9607_IAD_V00) || defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9607_IAD_V01)
	int total_leds = (sizeof(RTL9607)/sizeof(RTL9607[0]));
	int OP=0;

	for(i=0;i<total_leds;i++)
	{
		if(RTL9607[i].mp_led_type == mp_led_type)
		{ 
			if(RTL9607[i].mp_led_gpio_state) 
				OP = LED_OFF;//high active
			else
				OP = LED_ON;//low active
			PRINT("%s-%d  [%d].index=%d [%d].state=%d\n",__func__,__LINE__,i,RTL9607[i].mp_led_gpio_index,i,RTL9607[i].mp_led_gpio_state);
			MP_GPIO_SET(RTL9607[i].mp_led_gpio_index, OP);
		}
	}

#elif defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9602C)
	int total_leds = (sizeof(RTL9602)/sizeof(RTL9602[0]));
	int OP=0;

	for(i=0;i<total_leds;i++)
	{
		if(RTL9602[i].mp_led_type == mp_led_type)
		{ 
			if(RTL9602[i].mp_led_gpio_state) 
				OP = LED_OFF;//high active
			else
				OP = LED_ON;//low active
			PRINT("%s-%d  [%d].index=%d [%d].state=%d\n",__func__,__LINE__,i,RTL9602[i].mp_led_gpio_index,i,RTL9602[i].mp_led_gpio_state);
			MP_GPIO_SET(RTL9602[i].mp_led_gpio_index, OP);
		}
	}
#endif
	return -1;
}

void led_set_all_led_on(void)
{
#if defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9601B)
PRINT("LED ON\n");
mptest_on(RTL9601b,sizeof(RTL9601b)/sizeof(RTL9601b[0]));
#elif defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9607_IAD_V00) || defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9607_IAD_V01)
			PRINT("RTL9607 LED ON\n");
			mptest_on(RTL9607,sizeof(RTL9607)/sizeof(RTL9607[0]));
	#if (defined(CONFIG_SLOT_0_8812) || defined(CONFIG_SLOT_0_8812AR_VN))  && defined(CONFIG_WLAN0_5G_WLAN1_2G)
			wps_8812_led_on();
	#elif (defined(CONFIG_SLOT_0_8192EE) || defined(CONFIG_SLOT_1_8192EE))
			wps_8192_led_on();
	#endif
#elif defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9602C)
			PRINT("RTL9602C LED ON\n");
			mptest_on(RTL9602,sizeof(RTL9602)/sizeof(RTL9602[0]));
	#if (defined(CONFIG_SLOT_0_8812) || defined(CONFIG_SLOT_0_8812AR_VN))  && defined(CONFIG_WLAN0_5G_WLAN1_2G)
			wps_8812_led_on();
	#elif (defined(CONFIG_SLOT_0_8192EE) || defined(CONFIG_SLOT_1_8192EE))
			wps_8192_led_on();
	#endif		
#endif
/*TODO: any other led should be add here*/
}

void led_set_all_led_off(void)
{
	static unsigned int regValue;	
#if defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9601B)
				print_string("LED OFF\n");
				mptest_off(RTL9601b,sizeof(RTL9601b)/sizeof(RTL9601b[0]));
#elif defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9607_IAD_V00) || defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9607_IAD_V01)
			print_string("RTL9607 LED OFF\n");
			mptest_off(RTL9607,sizeof(RTL9607)/sizeof(RTL9607[0]));
	#if (defined(CONFIG_SLOT_0_8812) || defined(CONFIG_SLOT_0_8812AR_VN))  && defined(CONFIG_WLAN0_5G_WLAN1_2G)
			wps_8812_led_off();
	#elif (defined(CONFIG_SLOT_0_8192EE) || defined(CONFIG_SLOT_1_8192EE) )
			wps_8192_led_off();
	#endif
#elif defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9602C)	
			print_string("RTL9602 LED OFF\n");
			mptest_off(RTL9602,sizeof(RTL9602)/sizeof(RTL9602[0]));
	#if (defined(CONFIG_SLOT_0_8812) || defined(CONFIG_SLOT_0_8812AR_VN))  && defined(CONFIG_WLAN0_5G_WLAN1_2G)
			wps_8812_led_off();
	#elif (defined(CONFIG_SLOT_0_8192EE) || defined(CONFIG_SLOT_1_8192EE) )
			wps_8192_led_off();
	#endif	
#endif			
				/*TODO: any other led should be add here*/

}

static int led_test_write(struct file *file, const char __user *buffer, size_t len, loff_t *ppos)
{
	rtk_led_config_t ledConfig;
	char	tmpbuf[512];
	char	*strptr, *tokptr;
	static unsigned int regValue;
#if defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9602C)
	static unsigned int gpio_restore_value_48,gpio_restore_value_4c;
#endif

	if (buffer && !copy_from_user(tmpbuf, buffer, len))
	{
		tmpbuf[len] = '\0';
		
		strptr=tmpbuf;

		if(strlen(strptr)==0)
			goto errout;
		tokptr = strsep(&strptr," ");
		if (tokptr == NULL)
			goto errout;
		
		/*parse command*/
		if(strncmp(tokptr, "start", 5) == 0)
		{
			led_test_start = 1;
			struct led_gpio_struct* led=NULL;
			int len;
#if defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9607_IAD_V00)||defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9607_IAD_V01)
			led=RTL9607;
			len=sizeof(RTL9607)/sizeof(RTL9607[0]);
#elif defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9602C)
			led=RTL9602;
			len=sizeof(RTL9602)/sizeof(RTL9602[0]);
#endif
			if(led!=NULL && len!=0)
				save_led_state(led, len);
#if defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9602C)
			if(0x3000 != REG32(0xBB023014))
				regValue = REG32(0xBB023014); //record current status
			REG32(0xBB023014) =  0x3000;
			if(0 != REG32(0xBB000048))
				gpio_restore_value_48 = REG32(0xBB000048);
			if(0 != REG32(0xBB00004C))
				gpio_restore_value_4c = REG32(0xBB00004C);
#else
			if(0 != REG32(0xBB023014))
				regValue = REG32(0xBB023014); //record current status
			REG32(0xBB023014) = 0;	
#endif
		}
		else if(strncmp(tokptr, "stop", 4) == 0)
		{
			led_test_start = 0;
			/* clear all button trace */
#ifdef CONFIG_WLAN_ON_OFF_BUTTON			
			wlan_onoff_flag = '0';
#endif
			default_flag = '0';
#if defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9607_IAD_V00)||defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9607_IAD_V01)
			restore_led_state(RTL9607,sizeof(RTL9607)/sizeof(RTL9607[0]));
#elif defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9602C)
			restore_led_state(RTL9602,sizeof(RTL9602)/sizeof(RTL9602[0]));
#endif
#if defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9602C)
			REG32(0xBB023014) = regValue;
			REG32(0xBB000048) = gpio_restore_value_48;
			REG32(0xBB00004c) = gpio_restore_value_4c;
#else
			REG32(0xBB023014) = regValue;
#endif
			/*TODO, we should add wps button later*/
		}
		else if (strncmp(tokptr, "reset", 5) == 0)
		{
			if (0 == led_test_start)
				goto errout;
			default_flag = '1';
		}
#ifdef CONFIG_USB_SUPPORT		
		else if(strncmp(tokptr, "usbtest", 7) == 0)
		{
		}
#endif		
		else if (strncmp(tokptr, "ethstatus", 9) == 0)
		{
			ethtest = 1;
		}
		else if (strncmp(tokptr, "set", 3) == 0)
		{
			if (0 == led_test_start)
				goto errout;
			
			tokptr = strsep(&strptr," ");
			if (tokptr==NULL)
				goto errout;
			
			if ( !strncmp(tokptr, "power", 5)) {
				tokptr = strsep(&strptr," ");
				if (tokptr==NULL)
					goto errout;

				if ( !strncmp(tokptr, "green", 5)) {
					led_on(LED_POWER_GREEN);
				}
				else if ( !strncmp(tokptr, "red", 3)) {
					led_on(LED_POWER_RED);
				}
			}
			else if ( !strncmp(tokptr, "internet", 8)) {
				tokptr = strsep(&strptr," ");
				if (tokptr==NULL)
					goto errout;

				if ( !strncmp(tokptr, "green", 5)) {
					led_on(LED_INTERNET_GREEN);
				}
				else if ( !strncmp(tokptr, "red", 3)) {
					led_on(LED_INTERNET_RED);
				}
			}
			else if ( !strncmp(tokptr, "lan", 3)) {
				int port;
				
				tokptr = strsep(&strptr," ");
				if (tokptr==NULL)
					goto errout;
				port = simple_strtol(tokptr, NULL, 0);
#if defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9601B)
				if ((port>1) || (port<1))
#else
				if ((port>4) || (port<1))
#endif					
					goto errout;
				printk("port:%d\n",port);
				switch(port){
					case 1:
						led_set_single_light(LED_LAN0);
						break;
					case 2:
						led_set_single_light(LED_LAN1);
						break;
					case 3:
						led_set_single_light(LED_LAN2);
						break;
					case 4:
						led_set_single_light(LED_LAN3);
						break;
					default:
						printk("Error Port=%d\n", port);
						break;
				}
				
			}
			else if( !strncmp(tokptr, "pon", 3)) {
				led_set_single_light(LED_PON);
			}
			else if( !strncmp(tokptr, "los", 3)) {
				led_set_single_light(LED_LOS);
			}
#ifdef CONFIG_RTK_VOIP
			else if( !strncmp(tokptr, "fxs", 3)) {
				int port;
				tokptr = strsep(&strptr," ");
				if (tokptr==NULL)
					goto errout;
				port = simple_strtol(tokptr, NULL, 0);
				switch(port)
				{
					case 1:
						led_set_single_light(LED_FX0);
						break;
					case 2:
						led_set_single_light(LED_FX1);
						break;
					default:
						printk("Error Port=%d\n", port);
						break;
				}
			}				
#endif			
#if defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9601B)
			else if( !strncmp(tokptr, "1G", 2)) {
				led_set_single_light(LED_1G);
			}
#endif	
			else if ( !strncmp(tokptr, "dsl", 3)) {
				//TODO
			}
#ifndef CONFIG_PON_LED_PROFILE_DEMO_RTL9601B			
			else if ( !strncmp(tokptr, "wlan0", 5)) {
#if defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9607_IAD_V00) && defined (CONFIG_RTL8192CD)
				MP_GPIO_SET(RTL8192CD_GPIO_5, LED_ON); //wps_G
#elif defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9607_IAD_V01) && defined (CONFIG_RTL8192CD)
                MP_GPIO_SET(RTL8192CD_GPIO_5, LED_ON); //wps_G
#elif defined(CONFIG_PON_LED_PROFILE_DEMO_RTL9602C) && defined (CONFIG_RTL8192CD)	
				MP_GPIO_SET(RTL8192CD_GPIO_5, LED_ON); //wps_G
#endif
				led_on(LED_WLAN_2_4G);
			}
			else if ( !strncmp(tokptr, "wlan1", 5)) {
				led_on(LED_WLAN_5G);
			}
			else if ( !strncmp(tokptr, "wps", 2)) {
				tokptr = strsep(&strptr," ");
				if (tokptr==NULL)
					goto errout;

				if ( !strncmp(tokptr, "green", 5)) {
					led_on(LED_WPS_GREEN);
				}
			}
#ifdef CONFIG_USB_SUPPORT			
			else if ( !strncmp(tokptr, "usbhost", 7)) {
#ifdef CONFIG_SW_USB_LED0
				led_on(LED_USB_0);
#endif
#ifdef CONFIG_SW_USB_LED1
				led_on(LED_USB_1);
#endif
			}
#endif			
#endif /*CONFIG_RTL9601B_SERIES*/			
		}
		else if (strncmp(tokptr, "allledon", 8) == 0) {
			if (0 == led_test_start)
				goto errout;
			led_set_all_led_on();
			/*TODO: any other led should be add here*/
		}
		else if (strncmp(tokptr, "allledoff", 9) == 0) {
			if (0 == led_test_start)
				goto errout;
			led_set_all_led_off();
			/*TODO: any other led should be add here*/
		}
		else if (strncmp(tokptr, "allgreenon", 10) == 0) {
			if (0 == led_test_start)
				goto errout;
			printk("Not Support yet!\n");
		}
		else if (strncmp(tokptr, "allgreenoff", 11) == 0) {
			if (0 == led_test_start)
				goto errout;
			printk("Not Support yet!\n");

		}
		else if (strncmp(tokptr, "allredon", 8) == 0) {
			printk("Not Support yet!\n");

/*
			led_on(LED_POWER_RED);
			led_on(LED_INTERNET_RED);
*/			
		}
		else if (strncmp(tokptr, "allredoff", 9) == 0) {
			printk("Not Support yet!\n");			
/*
			led_off(LED_POWER_RED);
			led_off(LED_INTERNET_RED);
*/			
		}
		else if(strncmp(tokptr, "clean", 5) == 0)
		{
			default_flag = '0';
#ifdef CONFIG_WLAN_ON_OFF_BUTTON			
			wlan_onoff_flag = '0';			
#endif
		}
		else
		{
			goto errout;
		}
	}
	else
	{
errout:
		printk("err command\n");
	}

	return len;
}


#endif

static int mptest_level_read(struct seq_file *seq, void *v)
{
	return 0;
}


static int mptest_open(struct inode *inode, struct file *file)
{
        return single_open(file, mptest_level_read, inode->i_private);
}
static const struct file_operations mptest_fops = {
        .owner          = THIS_MODULE,
        .open           = mptest_open,
        .read           = seq_read,
        .write          = mptest_level_write,
        .llseek         = seq_lseek,
        .release        = single_release,
};


static int led_test_open(struct inode *inode, struct file *file)
{
        return single_open(file, led_test_read, inode->i_private);
}
static const struct file_operations led_test_fops = {
        .owner          = THIS_MODULE,
        .open           = led_test_open,
        .read           = seq_read,
        .write          = led_test_write,
        .llseek         = seq_lseek,
        .release        = single_release,
};


static int __init mptest_init(void) {

	proc_create_data("mptest",0644, NULL,&mptest_fops, NULL);

#ifdef CONFIG_APOLLO_MP_TEST
	proc_create_data("led_test",0644, NULL,&led_test_fops, NULL);
#endif

	return 0;
}

static void __exit mptest_exit(void) {
}


module_init(mptest_init);
module_exit(mptest_exit);


MODULE_LICENSE("GPL");
