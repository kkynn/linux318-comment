#!/bin/sh
#Generate luna firmware upgarde image tar ball

up_script="fwu.sh"
fw_list="rootfs uImage fwu_ver hw_ver"
framework_list="framework.img framework.sh"
all_list="$up_script $fw_list $framework_list"
md5_result="md5.txt"
dst_dir="../images"
romfs_ver="../romfs/etc/version"
fw_tar_file="img.tar"
if [ "y" = "$CONFIG_CMCC" ] || [ "y" = "$CONFIG_CU" ]; then 
fw_tar_file2="img_enc.tar"
openssl_exe="openssl"
openssl_key="realtek"
encrypt_algo="aes-128-cbc"
fi
framework_tar_file="framework.tar"
all_tar_file="fw_framework.tar"

hw_check=`cat ../.config | grep CONFIG_HWVER_CHECK | sed s/CONFIG_HWVER_CHECK=//g | sed s/\"//g`
hw_ver_str=`cat ../.config | grep CONFIG_HW_HWVER | sed s/CONFIG_HW_HWVER=\"//g | sed s/\"//g`
is_yueme=`cat .config | grep "CONFIG_YUEME=" | sed s/[^y]//g | sed s/\"//g`
is_cmcc=`cat .config | grep "CONFIG_CMCC=" | sed s/[^y]//g | sed s/\"//g`
is_cu=`cat .config | grep "CONFIG_CU=" | sed s/[^y]//g | sed s/\"//g`

test -f hw_ver && rm -rf hw_ver
if [ "$hw_check" = "y" ]; then
    echo $hw_ver_str > hw_ver
else
    echo "skip" > hw_ver
fi

if [ "$is_cmcc" = "y" ]; then
    cp fwu_cmcc.sh fwu.sh
else
if [ "$is_cu" = "y" ]; then
    cp fwu_cmcc.sh fwu.sh
else
    cp fwu_sdk.sh fwu.sh
fi
fi

cp fwu.sh $dst_dir
cp fwu_ver $dst_dir

# If /romfs/etc/version is exist, replace it to /images/fwu_ver 
# then parse&store by nv setenv in fwu.sh
cat $romfs_ver
if [ $? = 0 ]; then	
	cat $romfs_ver > $dst_dir/fwu_ver
fi

cp hw_ver $dst_dir
cd $dst_dir
md5sum $fw_list $up_script > $md5_result
tar -cf $fw_tar_file $up_script $fw_list $md5_result
if [ "y" = "$CONFIG_CMCC" ] || [ "y" = "$CONFIG_CU" ]; then 
cat $fw_tar_file | $openssl_exe $encrypt_algo -e -out $fw_tar_file2  -k $openssl_key
fi

if [ "$is_yueme" = "y" ] && [ -f ../framework.img ]; then
	cd -
	echo Aligning framework.img to 2k/page boundary for NAND platform
	sz=`ls -l ../framework.img | cut -d ' ' -f5`
	pagecnt=$(( (sz+2047) / 2048 ))
	dd if=../framework.img ibs=2k count=$pagecnt of=$dst_dir/framework.img conv=sync
	ls -l ../images/framework.img
	cp framework.sh $dst_dir

	cd $dst_dir
	md5sum $framework_list $up_script > $md5_result
	tar -cf $framework_tar_file $up_script $framework_list $md5_result
	md5sum $all_list > $md5_result
	tar -cf $all_tar_file $all_list $md5_result
fi

ls -l $img_tar_file
