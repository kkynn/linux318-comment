#!/bin/sh

pon_mode=`flash get PON_MODE`
if [ "$pon_mode" == "PON_MODE=1" ]; then
	if [ ! -f /var/config/run_customized_sdk.sh ]; then
		cp /etc/run_customized_sdk.sh /var/config/run_customized_sdk.sh
	fi
		/var/config/run_customized_sdk.sh
		/etc/runomci.sh
		echo "running GPON mode ..."
elif [ "$pon_mode" == "PON_MODE=2" ]; then
        modprobe ca-rtk-epon-drv
        /etc/runoam.sh
        sleep 2
        echo "running EPON mode ..."
elif [ "$pon_mode" == "PON_MODE=3" ]; then
        echo "running Fiber mode (not support at CA chip)..."
elif echo $pon_mode | grep -q "GET fail"; then
        echo "running Fiber mode (not support at CA chip)..."
else
        echo "running Ether mode..."
fi

