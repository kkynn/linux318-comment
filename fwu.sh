#!/bin/sh

# luna firmware upgrade  script
# $1 image destination (0 or 1) 
# Kernel and root file system images are assumed to be located at the same directory named uImage and rootfs respectively
# ToDo: use arugements to refer to kernel/rootfs location.

k_img="uImage"
r_img="rootfs"
img_ver="fwu_ver"
md5_cmp="md5.txt"
md5_cmd="/bin/md5sum"
#md5 run-time result
md5_tmp="md5_tmp" 
md5_rt_result="md5_rt_result.txt"
new_fw_ver="new_fw_ver.txt"
cur_fw_ver="cur_fw_ver.txt"
env_sw_ver="env_sw_ver.txt"
hw_ver_file="hw_ver"
skip_hwver_check="/tmp/skip_hwver_check"

# For YueMe framework
framework_img="framework.img"
framework_sh="framework.sh"
framework_upgraded=0

# Stop this script upon any error
# set -e

if [ "`tar -tf $2 $framework_sh`" = "$framework_sh" ] && [ "`tar -tf $2 $framework_img`" = "$framework_img" ]; then
    echo "Updaing framework from $2"
    tar -xf $2 $framework_sh
    grep $framework_sh $md5_cmp > $md5_tmp
    $md5_cmd $framework_sh > $md5_rt_result
    diff $md5_rt_result $md5_tmp

    if [ $? != 0 ]; then 
        echo "$framework_sh md5_sum inconsistent, aborted image updating !"
        exit 1
    fi

    # Run firmware upgrade script extracted from image tar ball
    sh $framework_sh $2
    framework_upgraded=1
fi

if [ "`tar -tf $2 $k_img`" = '' ] && [ $framework_upgraded = 1 ]; then
    echo "No uImage for upgrading, skip"
    exit 2
fi

echo "Updating image $1 with file $2"

# Find out kernel/rootfs mtd partition according to image destination
k_mtd="/dev/"`cat /proc/mtd | grep \"k"$1"\" | sed 's/:.*$//g'`
r_mtd="/dev/"`cat /proc/mtd | grep \"r"$1"\" | sed 's/:.*$//g'`
k_mtd_size=`cat /proc/mtd | grep \"k"$1"\" | sed 's/^.*: //g' | sed 's/ .*$//g'`
r_mtd_size=`cat /proc/mtd | grep \"r"$1"\" | sed 's/^.*: //g' | sed 's/ .*$//g'`
echo "kernel image is located at $k_mtd"
echo "rootfs image is located at $r_mtd"

if [ -f $skip_hwver_check ]; then
    echo "Skip HW_VER check!!"
else
    img_hw_ver=`tar -xf $2 $hw_ver_file -O`
    mib_hw_ver=`flash get HW_HWVER | sed s/HW_HWVER=//g`
    if [ "$img_hw_ver" = "skip" ]; then
        echo "skip HW_VER check!!"
    else
        echo "img_hw_ver=$img_hw_ver mib_hw_ver=$mib_hw_ver"
        if [ "$img_hw_ver" != "$mib_hw_ver" ]; then
            echo "HW_VER $img_hw_ver inconsistent, aborted image updating !"
            exit 1
        fi
    fi
fi

# Extract kernel image
tar -xf $2 $k_img -O | md5sum | sed 's/-/'$k_img'/g' > $md5_rt_result
# Check integrity
grep $k_img $md5_cmp > $md5_tmp
diff $md5_rt_result $md5_tmp

if [ $? != 0 ]; then
    echo "$k_img""md5_sum inconsistent, aborted image updating !"
    exit 1
fi

# Extract rootfs image
tar -xf $2 $r_img -O | md5sum | sed 's/-/'$r_img'/g' > $md5_rt_result
# Check integrity
grep $r_img $md5_cmp > $md5_tmp
diff $md5_rt_result $md5_tmp

if [ $? != 0 ]; then
    # rm $r_img
    echo "$r_img""md5_sum inconsistent, aborted image updating !"
    exit 1
fi

echo "Integrity of $k_img & $r_img is okay."

# Check upgrade firmware's version with current firmware version
tar -xf $2 $img_ver
if [ $? != 0 ]; then
	echo "1" > /var/firmware_upgrade_status
	echo "Firmware version incorrect: no fwu_ver in img.tar !"
	exit 1
fi

cat $img_ver > $new_fw_ver
cat /etc/version > $cur_fw_ver

cat $new_fw_ver | grep -n '^V[0-9]*.[0-9]*.[0-9]*-[0-9][0-9]*'
if [ $? != 0 ]; then
	echo "1" > /var/firmware_upgrade_status
	echo "Firmware version incorrect: `cat $new_fw_ver` !"
	exit 1
fi

echo "Try to upgrade firmware version from `cat $cur_fw_ver`"
echo "                                  to `cat $new_fw_ver`"

if [ "`cat $new_fw_ver`" == "`cat $cur_fw_ver`" ]; then
	echo "4" > /var/firmware_upgrade_status
    echo "Current firmware version already is `cat $cur_fw_ver` !"
    exit 1
fi

echo "Firware version check okay."

tar -xf $2 $k_img
string="`ls -l | grep $k_img`"
mtd_size_dec="`printf %d 0x$k_mtd_size`"
img_size_dec="`expr substr "$string" 34 100 | sed 's/^ *//g' | sed 's/ .*$//g'`"
expr "$img_size_dec" \< "$mtd_size_dec" > /dev/null
if [ $? != 0 ]; then
	echo "uImage size too big($img_size_dec) !"
	echo "3" > /var/firmware_upgrade_status
	exit 1
fi
tar -xf $2 $r_img
string="`ls -l | grep $r_img`"
mtd_size_dec="`printf %d 0x$r_mtd_size`"
img_size_dec="`expr substr "$string" 34 100 | sed 's/^ *//g' | sed 's/ .*$//g'`"
expr "$img_size_dec" \< "$mtd_size_dec" > /dev/null
if [ $? != 0 ]; then
	echo "rootfs size too big($img_size_dec) !"
	echo "3" > /var/firmware_upgrade_status
	exit 1
fi

echo "Both uImage and rootfs size check okay, start updating ..."
# Erase kernel partition 
flash_eraseall $k_mtd
# Write kernel partition
echo "Writing $k_img to $k_mtd"
cp $k_img $k_mtd

# Erase rootfs partition 
flash_eraseall $r_mtd
# Write rootfs partition
echo "Writing $r_img to $r_mtd"
cp $r_img $r_mtd


cat $new_fw_ver | grep CST
if [ $? = 0 ]; then
	echo `cat $new_fw_ver` | sed 's/ *--.*$//g' > $env_sw_ver
else
	cat $new_fw_ver > $env_sw_ver
fi
# Write image version information 
nv setenv sw_version"$1" "`cat $env_sw_ver`"

# Clean up temporary files
rm -f $md5_cmp $md5_tmp $md5_rt_result $img_ver $new_fw_ver $cur_fw_ver $env_sw_ver $k_img $r_img $2

# Post processing (for future extension consideration)

echo "Successfully updated image $1!!"

