#!/bin/sh

if [ -d /proc/rg ]; then
	rg_igmp=1
else
	rg_igmp=0
fi

igmp_conf_debug=`cat /var/config/igmp_gpon.conf | grep DEBUG_MODE | sed 's/# *DEBUG_MODE=//g'`

create_igmp_conf ()
{
	if [ ! -f /var/config/igmp_gpon.conf ] || [ "$igmp_conf_debug" = "0" ]; then
		cp /etc/igmp_gpon.conf /var/config/igmp_gpon.conf
	fi
}

if [ "$rg_igmp" == "0" ]; then
	create_igmp_conf
	insmod /lib/modules/igmp_drv.ko
	wait 1
	igmpd -f /var/config/igmp_gpon.conf &
fi
