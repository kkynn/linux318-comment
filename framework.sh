#!/bin/sh

# YueMe framework update script

fw_img="framework.img"
md5_cmp="md5.txt"
md5_cmd="/bin/md5sum"
#md5 run-time result
md5_tmp="md5_tmp" 
md5_rt_result="md5_rt_result.txt"

tar -xf $1 $md5_cmp 
if [ $? != 0 ]; then 
    echo "Failed to extract $1, aborted image updating !"
    exit 1
fi

# Stop this script upon any error
# set -e
echo "Pausing framework......"
dbus-send --system --print-reply --dest=com.ctc.saf1 /com/ctc/saf1 com.ctc.saf1.framework.Pause uint32:"10"

echo "Updating framework with file $1"

# Find out kernel/rootfs mtd partition according to image destination
fw_mtd="mtd"`dbus-send --system --print-reply --dest=com.ctc.saf1 /com/ctc/saf1 com.ctc.igd1.Properties.Get string:"com.ctc.saf1.framework" string:"BackupMTD" | grep "variant" | tr -d -c 0-9`
fw_mtd_size=`cat /proc/mtd | grep $fw_mtd | sed 's/^.*: //g' | sed 's/ .*$//g'`
echo "framework image is located at $fw_mtd size is $fw_mtd_size"

# Extract framework image
tar -xf $1 $fw_img -O | md5sum | sed 's/-/'$fw_img'/g' > $md5_rt_result
# Check integrity
grep $fw_img $md5_cmp > $md5_tmp
diff $md5_rt_result $md5_tmp

if [ $? != 0 ]; then
    echo "$k_img""md5_sum inconsistent, aborted image updating !"
    exit 1
fi

echo "Integrity of $fw_img is okay."

tar -xf $1 $fw_img
string="`ls -l | grep $fw_img`"
mtd_size_dec="`printf %d 0x$fw_mtd_size`"
img_size_dec="`expr substr "$string" 34 100 | sed 's/^ *//g' | sed 's/ .*$//g'`"
expr "$img_size_dec" \< "$mtd_size_dec" > /dev/null
if [ $? != 0 ]; then
	echo "$fw_img size too big($img_size_dec) !"
	echo "3" > /var/firmware_upgrade_status
	exit 1
fi

echo "YueMe framework size check okay, start updating ..."

# Erase kernel partition 
flash_eraseall /dev/$fw_mtd
# Write kernel partition
echo "Writing $fw_img to /dev/$fw_mtd"
cp $fw_img /dev/$fw_mtd

#Update
dbus-send --system --print-reply --dest=com.ctc.saf1 /com/ctc/saf1 com.ctc.saf1.framework.UpdateMTDInfo

#Restart framework
echo "Restarting framework......"
dbus-send --system --print-reply --dest=com.ctc.saf1 /com/ctc/saf1 com.ctc.saf1.framework.Pause uint32:"0"

# Post processing (for future extension consideration)
echo "Successfully updated framework!!"

