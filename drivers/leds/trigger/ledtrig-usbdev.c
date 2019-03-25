/*
 * LED USB device Trigger
 *
 * Toggles the LED to reflect the presence and activity of an USB device
 * Copyright (C) Gabor Juhos <juhosg@openwrt.org>
 *
 * derived from ledtrig-netdev.c:
 *	Copyright 2007 Oliver Jowett <oliver@opencloud.com>
 *
 * ledtrig-netdev.c derived from ledtrig-timer.c:
 *	Copyright 2005-2006 Openedhand Ltd.
 *	Author: Richard Purdie <rpurdie@openedhand.com>
 *
 * 2017-01-18  Andrew Chang <yachang@realtek.com>
 *  Modified to support multiple dev 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
 
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <linux/timer.h>
#include <linux/ctype.h>
#include <linux/slab.h>
#include <linux/leds.h>
#include <linux/usb.h>

#include "../leds.h"

#define DEV_BUS_ID_SIZE		32

/*
 * Configurable sysfs attributes:
 *
 * device_name - name of the USB device to monitor
 * activity_interval - duration of LED blink, in milliseconds
 */

#define DEV_NUM	4

struct mdevice { 
	char device_name[DEV_BUS_ID_SIZE];
	int last_urbnum;
	struct usb_device *usb_dev;
};
 
struct usbdev_trig_data {
	rwlock_t lock;

	struct timer_list timer;
	struct notifier_block notifier;

	struct led_classdev *led_cdev;	
	unsigned interval;	
	struct mdevice devs[DEV_NUM];
	int ndevs;
};

static inline int num_active_dev(struct usbdev_trig_data *td) {
	int i, n = 0;
	for (i = 0; i < td->ndevs; i++) 
		if (td->devs[i].usb_dev)
			n++;
	return n;
}

static void usbdev_trig_update_state(struct usbdev_trig_data *td)
{
	int n; 
	n = num_active_dev(td);
	if (n)
		led_set_brightness(td->led_cdev, LED_FULL);
	else
		led_set_brightness(td->led_cdev, LED_OFF);

	if (td->interval && n)
		mod_timer(&td->timer, jiffies + td->interval);
	else
		del_timer(&td->timer);
}

static ssize_t usbdev_trig_name_show(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct usbdev_trig_data *td = led_cdev->trigger_data;
	int n, len = 0;
	
	read_lock(&td->lock);	
	for (n=0; n<td->ndevs; n++) {
		if (n!=(td->ndevs-1))
			len += sprintf(&buf[len],"%s,",td->devs[n].device_name);
		else 
			len += sprintf(&buf[len],"%s\n",td->devs[n].device_name);
		
	}	
	read_unlock(&td->lock);

	return len + 1;	
}

struct usbdev_trig_match {
	char *device_name;
	struct usb_device *usb_dev;
};

static int usbdev_trig_find_usb_dev(struct usb_device *usb_dev, void *data)
{
	struct usbdev_trig_match *match = data;

	if (WARN_ON(match->usb_dev))
		return 0;

	if (!strcmp(dev_name(&usb_dev->dev), match->device_name)) {
		dev_dbg(&usb_dev->dev, "matched this device!\n");
		match->usb_dev = usb_get_dev(usb_dev);
	}

	return 0;
}

static ssize_t usbdev_trig_name_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf,
				      size_t size)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct usbdev_trig_data *td = led_cdev->trigger_data;
	const char *devnames[DEV_NUM], *tmp0, *tmp1;		
	int devnamelens[DEV_NUM], n, ndevs=0, stop=0;	

	if (size < 0 || size >= (DEV_BUS_ID_SIZE * DEV_NUM))
		return -EINVAL;

	tmp0=buf;	
	while (!stop) {
		tmp1=strchr(tmp0,',');
		if (!tmp1) {
			if (NULL==(tmp1=strchr(tmp0,'\n')))
				tmp1=strchr(tmp0,'\0');
			stop=1;
		}		
		devnames[ndevs]=tmp0;
		devnamelens[ndevs]=tmp1-tmp0;
		if ((devnamelens[ndevs] >= DEV_BUS_ID_SIZE) || (devnamelens[ndevs] == 0))
			return -EINVAL;
		tmp0=tmp1+1;
		ndevs++;
		if (ndevs >= DEV_NUM)
			stop=1;
	}

	write_lock(&td->lock);	
	td->ndevs = ndevs;
	if (ndevs > 0) {
		for (n = 0; n < td->ndevs; n++) {
			struct usbdev_trig_match match; 
			strncpy(td->devs[n].device_name, devnames[n], devnamelens[n]);
			memset(&match, 0, sizeof(match));
			match.device_name = td->devs[n].device_name;
			
			usb_for_each_dev(&match, usbdev_trig_find_usb_dev);
			if (match.usb_dev) {
				if (td->devs[n].usb_dev)
					usb_put_dev(td->devs[n].usb_dev);
				td->devs[n].usb_dev = match.usb_dev;
				td->devs[n].last_urbnum = atomic_read(&match.usb_dev->urbnum);
			}
		}
		usbdev_trig_update_state(td);
	}
	write_unlock(&td->lock);
	return size;
}

static DEVICE_ATTR(device_name, 0644, usbdev_trig_name_show,
		   usbdev_trig_name_store);

static ssize_t usbdev_trig_interval_show(struct device *dev,
				 	 struct device_attribute *attr,
					 char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct usbdev_trig_data *td = led_cdev->trigger_data;

	read_lock(&td->lock);
	sprintf(buf, "%u\n", jiffies_to_msecs(td->interval));
	read_unlock(&td->lock);

	return strlen(buf) + 1;
}

static ssize_t usbdev_trig_interval_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf,
					  size_t size)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct usbdev_trig_data *td = led_cdev->trigger_data;
	int ret = -EINVAL;
	char *after;
	unsigned long value = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (*after && isspace(*after))
		count++;

	if (count == size && value <= 10000) {
		write_lock(&td->lock);
		td->interval = msecs_to_jiffies(value);
		usbdev_trig_update_state(td); /* resets timer */
		write_unlock(&td->lock);
		ret = count;
	}

	return ret;
}

static DEVICE_ATTR(activity_interval, 0644, usbdev_trig_interval_show,
		   usbdev_trig_interval_store);

static int usbdev_trig_notify(struct notifier_block *nb,
			      unsigned long evt,
			      void *data)
{
	struct usb_device *usb_dev;
	struct usbdev_trig_data *td;
	int n;

	if (evt != USB_DEVICE_ADD && evt != USB_DEVICE_REMOVE)
		return NOTIFY_DONE;

	usb_dev = data;
	td = container_of(nb, struct usbdev_trig_data, notifier);

	write_lock(&td->lock);

	for (n = 0; n < td->ndevs; n++) {
		if (!strcmp(dev_name(&usb_dev->dev), td->devs[n].device_name)) {			
			break;
		}
	}
	if (n>=td->ndevs)
		goto done;

	if (evt == USB_DEVICE_ADD) {
		usb_get_dev(usb_dev);
		if (td->devs[n].usb_dev != NULL)
			usb_put_dev(td->devs[n].usb_dev);
		td->devs[n].usb_dev = usb_dev;
		td->devs[n].last_urbnum = atomic_read(&usb_dev->urbnum);
	} else if (evt == USB_DEVICE_REMOVE) {
		if (td->devs[n].usb_dev != NULL) {
			usb_put_dev(td->devs[n].usb_dev);
			td->devs[n].usb_dev = NULL;
		}
	}

	usbdev_trig_update_state(td);

done:
	write_unlock(&td->lock);
	return NOTIFY_DONE;
}

static int dev_has_activity(struct usbdev_trig_data *td) {
	int i, n = 0;
	for (i = 0; i < td->ndevs; i++) {
		int urbnum;
		
		if (!td->devs[i].usb_dev)
			continue;
		
		urbnum = atomic_read(&td->devs[i].usb_dev->urbnum);
		if (urbnum!=td->devs[i].last_urbnum) {
			n++;
			td->devs[i].last_urbnum = urbnum;
		}			
	}
	return n;
}
/* here's the real work! */
static void usbdev_trig_timer(unsigned long arg)
{
	struct usbdev_trig_data *td = (struct usbdev_trig_data *)arg;
	int n; 
	
	write_lock(&td->lock);
	
	n = num_active_dev(td);

	if (n == 0 || td->interval == 0) {
		/*
		 * we don't need to do timer work, just reflect device presence
		 */
		if (n)
			led_set_brightness(td->led_cdev, LED_FULL);
		else
			led_set_brightness(td->led_cdev, LED_OFF);

		goto no_restart;
	}
	
	if (td->led_cdev->brightness == LED_OFF)
		led_set_brightness(td->led_cdev, LED_FULL);
	else if (dev_has_activity(td))
		led_set_brightness(td->led_cdev, LED_OFF);
	
	mod_timer(&td->timer, jiffies + td->interval);

no_restart:
	write_unlock(&td->lock);
}

static void usbdev_trig_activate(struct led_classdev *led_cdev)
{
	struct usbdev_trig_data *td;
	int rc;

	td = kzalloc(sizeof(struct usbdev_trig_data), GFP_KERNEL);
	if (!td)
		return;

	rwlock_init(&td->lock);

	td->notifier.notifier_call = usbdev_trig_notify;
	td->notifier.priority = 10;

	setup_timer(&td->timer, usbdev_trig_timer, (unsigned long) td);

	td->led_cdev = led_cdev;
	td->interval = msecs_to_jiffies(50);

	led_cdev->trigger_data = td;

	rc = device_create_file(led_cdev->dev, &dev_attr_device_name);
	if (rc)
		goto err_out;

	rc = device_create_file(led_cdev->dev, &dev_attr_activity_interval);
	if (rc)
		goto err_out_device_name;

	usb_register_notify(&td->notifier);
		
	do {
		char default_dev[] = "1-1,2-1,3-1,4-1";
		usbdev_trig_name_store(led_cdev->dev, 0, default_dev, sizeof(default_dev));
	} while (0);
	
	return;

err_out_device_name:
	device_remove_file(led_cdev->dev, &dev_attr_device_name);
err_out:
	led_cdev->trigger_data = NULL;
	kfree(td);
}

static void usbdev_trig_deactivate(struct led_classdev *led_cdev)
{
	struct usbdev_trig_data *td = led_cdev->trigger_data;
	int n;
	
	if (td) {
		usb_unregister_notify(&td->notifier);

		device_remove_file(led_cdev->dev, &dev_attr_device_name);
		device_remove_file(led_cdev->dev, &dev_attr_activity_interval);

		write_lock(&td->lock);

		for (n = 0; n < td->ndevs; n++) {
			if (td->devs[n].usb_dev) {
				usb_put_dev(td->devs[n].usb_dev);
				td->devs[n].usb_dev = NULL;
			}
		}

		write_unlock(&td->lock);

		del_timer_sync(&td->timer);

		kfree(td);
	}
}

static struct led_trigger usbdev_led_trigger = {
	.name		= "usbdev",
	.activate	= usbdev_trig_activate,
	.deactivate	= usbdev_trig_deactivate,
};

static int __init usbdev_trig_init(void)
{
	return led_trigger_register(&usbdev_led_trigger);
}

static void __exit usbdev_trig_exit(void)
{
	led_trigger_unregister(&usbdev_led_trigger);
}

module_init(usbdev_trig_init);
module_exit(usbdev_trig_exit);

MODULE_AUTHOR("Gabor Juhos <juhosg@openwrt.org>");
MODULE_DESCRIPTION("USB device LED trigger");
MODULE_LICENSE("GPL v2");
