#!/bin/sh

usage()
{
	printf "Usage: \n [Release]:\tcmcc_omci_release_bin.sh [all|custom|both] [rg|rtk|ca8279|all] v1.9.1.4 v1.9.1.4 v1.9.1.4\n"
	printf "\t\t all : all src to bin \n"
	printf "\t\t custom : only customizer src to bin \n"
	printf "\t\t both : both all and custom src to bin \n"
	printf "\t\t rg|rtk|ca8279 : release module \n"
	printf "\t\t 1st = omci user application version \n"
	printf "\t\t 2nd = omci rtk driver version\n"
	printf "\t\t 3th = omci rg driver version\n"
}

BUILD_OMCI_DIR=$(pwd | sed 's/\/TOOL.*//g')
BUILD_SDK_DIR=$(pwd |sed 's/\/linux.*//g')
echo BUILD_OMCI_DIR : ${BUILD_OMCI_DIR}
echo BUILD_SDK_DIR : ${BUILD_SDK_DIR}

DRV_RTK_DIR="DRV/platform/rtl9607_rtk"
DRV_RG_DIR="DRV/platform/rtl9607_rg"
DRV_CA8279_DIR="DRV/platform/ca8279"
DRV_FC_DIR="DRV/platform/rtl9607_fc"

function modVersion {

RELEASE_VERSION="\\\\\"$1\\\\\""
		
	sed -i '/^RELEASE_SUBVERSION/d' \
		"./Makefile" && test 0 -ne $? && exit 1
		
	if [ -z "`cat ./Makefile |\
		 grep "^RELEASE_VERSION" | head -n 1`" ]; then
		# Not exist RELEASE_VERSION variable
		sed -i "20 a RELEASE_VERSION\t\t\t= "${RELEASE_VERSION}""\
			 "./Makefile" && test 0 -ne $? && exit 1
	else
		# exist RELEASE_VERSION variable
		sed -i "s/^RELEASE_VERSION.*\"$/RELEASE_VERSION\t\t\t= \
			"${RELEASE_VERSION}"/g" "./Makefile" && test 0 -ne $? && exit 1
	fi

RTKDRV_RELEASE_VERSION="\\\\\"$2\\\\\""

	# for rtk drv release version
	if [ -z "`cat ./DRV/platform/rtl9607_rtk/Makefile |\
		 grep "^RTKDRV_RELEASE_VERSION" | head -n 1`" ]; then
		# Not exist RTKDRV_RELEASE_VERSION variable
		sed -i "3 a RTKDRV_RELEASE_VERSION\t\t\t= "${RTKDRV_RELEASE_VERSION}""\
			 "./DRV/platform/rtl9607_rtk/Makefile" && test 0 -ne $? && exit 1
	else
		# exist RTKDRV_RELEASE_VERSION variable
		sed -i "s/^RTKDRV_RELEASE_VERSION.*\"$/RTKDRV_RELEASE_VERSION\t\t\t= \
			"${RTKDRV_RELEASE_VERSION}"/g" \
			"./DRV/platform/rtl9607_rtk/Makefile" && test 0 -ne $? && exit 1
	fi

RGDRV_RELEASE_VERSION="\\\\\"$3\\\\\""

	# for rg  drv release version
	if [ -z "`cat ./DRV/platform/rtl9607_rg/Makefile |\
		 grep "^RGDRV_RELEASE_VERSION" | head -n 1`" ]; then
		# Not exist RTKDRV_RELEASE_VERSION variable
		sed -i "3 a RGDRV_RELEASE_VERSION\t\t\t= "${RGDRV_RELEASE_VERSION}""\
			 "./DRV/platform/rtl9607_rg/Makefile" && test 0 -ne $? && exit 1
	else
		# exist RTKDRV_RELEASE_VERSION variable
		sed -i "s/^RGDRV_RELEASE_VERSION.*\"$/RGDRV_RELEASE_VERSION\t\t\t= \
			"${RGDRV_RELEASE_VERSION}"/g" \
			"./DRV/platform/rtl9607_rg/Makefile" && test 0 -ne $? && exit 1
	fi
 
	# for fc  drv release version
	if [ -z "`cat ./DRV/platform/rtl9607_fc/Makefile |\
		 grep "^RGDRV_RELEASE_VERSION" | head -n 1`" ]; then
		# Not exist RTKDRV_RELEASE_VERSION variable
		sed -i "3 a RGDRV_RELEASE_VERSION\t\t\t= "${RGDRV_RELEASE_VERSION}""\
			 "./DRV/platform/rtl9607_fc/Makefile" && test 0 -ne $? && exit 1
	else
		# exist RTKDRV_RELEASE_VERSION variable
		sed -i "s/^RGDRV_RELEASE_VERSION.*\"$/RGDRV_RELEASE_VERSION\t\t\t= \
			"${RGDRV_RELEASE_VERSION}"/g" \
			"./DRV/platform/rtl9607_fc/Makefile" && test 0 -ne $? && exit 1
	fi

}

function all2bin {
echo $1 $2 $3
cd ${BUILD_OMCI_DIR}
BIN_OMCI_APP="omci_app"
BIN_OMCI_CLI="omcicli"

SVN_BIN_MK_FILE="http://cadinfo.realtek.com.tw/svn/CN/Switch/PON/public/applications/rtk_omci_release/BIN_OMCI_TEMPLATE.mak"

	rm -rf ./LIB/lib*
	rm -rf ./LIB/omci/mib*

# prepare binary OMCI
	find . -name "*.c" | xargs -n2 rm && test 0 -ne $? && exit 1
	find . -name "*.o" | xargs -n2 rm && test 0 -ne $? && exit 1
	find . -name "*.d" | xargs -n2 rm && test 0 -ne $? && exit 1
	rm -rf "./LIB/omci/*.so" && test 0 -ne $? && exit 1
	find "./DRV" -name "*.cmd" | xargs -n2 rm && test 0 -ne $? && exit 1
	find "./DRV" -name "modules.order" | xargs -n2 rm && test 0 -ne $? && exit 1
	find "./DRV" -name "Module.symvers" | xargs -n2 rm && test 0 -ne $? && exit 1
	find "./DRV" -name ".tmp_versions" | xargs -n2 rm -rf
	find "./DRV" -name ".mod" | xargs -n2 rm && test 0 -ne $? && exit 1

# All binaray files will be removed if make clean from sdk path. 
#Therefore, change name of omci binaray file 
	find . -name "*.so" | \
		sed -e p -e "s/so/so."${1#v*}.${2#v*}.${3#v*}"/g" | xargs -n2 mv && test 0 -ne $? && exit 1

	find . -name "*.ko" | \
		sed -e p -e "s/ko/ko_"${1#v*}.${2#v*}.${3#v*}"/g" | xargs -n2 mv && test 0 -ne $? && exit 1

	mv "./${BIN_OMCI_APP}" \
		"./${BIN_OMCI_APP}.${1#v*}.${2#v*}.${3#v*}" && test 0 -ne $? && exit 1

	mv "./${BIN_OMCI_CLI}" \
		"./${BIN_OMCI_CLI}.${1#v*}.${2#v*}.${3#v*}" && test 0 -ne $? && exit 1

	rm -f ./Makefile && test 0 -ne $? && exit 1
			
# prepare Makefile for binary OMCI
MAKEFILE="BIN_OMCI_TEMPLATE.mak"
svn export ${SVN_BIN_MK_FILE} ./Makefile && test 0 -ne $? && exit 1

	if [ -z "`cat ./Makefile | \
		grep "^RELEASE_VERSION" | head -n 1`" ]; then
		# Not exist RELEASE_VERSION variable
		sed -i "18 a RELEASE_VERSION="${1#v*}.${2#v*}.${3#v*}"" \
			./Makefile && test 0 -ne $? && exit 1
	else
		# exist RELEASE_VERSION variable
		sed -i "s/^RELEASE_VERSION.*/RELEASE_VERSION="${1#v*}.${2#v*}.${3#v*}"/g"\
			 ./Makefile && test 0 -ne $? && exit 1
	fi
}

function cus2bin {
echo $1 $2 $3
OMCI_VERSION="${1}"
find "${BUILD_OMCI_DIR}/DRV/features" -name "*.ko" | sed -e p -e "s/ko/ko_${OMCI_VERSION}/g" | xargs -n2 mv && test 0 -ne $? && exit 1
find "${BUILD_OMCI_DIR}/FAL/src/features" -name "*.so" | sed -e p -e "s/so/so.${OMCI_VERSION}/g" | xargs -n2 mv && test 0 -ne $? && exit 1
cd ${BUILD_SDK_DIR}
make clean && test 0 -ne $? && exit 1
find "${BUILD_OMCI_DIR}/FAL/src/features" -name "*.c" | xargs -n2 rm && test 0 -ne $? && exit 1
find "${BUILD_OMCI_DIR}/FAL/src/cfg" -name "*.c" | xargs -n2 rm && test 0 -ne $? && exit 1
find "${BUILD_OMCI_DIR}/DRV/features" -name "*.c" | xargs -n2 rm && test 0 -ne $? && exit 1
grep -v "^#" ${BUILD_SDK_DIR}/${OMCI_PATH}/openCustomized.lst > tmpOpenlst
while read svnline;do
if [ "$svnline" == "" ];then
continue 
fi
#remove open customized releated
find "${BUILD_OMCI_DIR}" -name "*$svnline*" | xargs -n2 rm -rf && test 0 -ne $? && exit 1
#restore open customized .c file
cd ${BUILD_OMCI_DIR}
svn up ./DRV/features/$svnline/
svn up ./FAL/src/features/$svnline.c
done < tmpOpenlst
}

function chk_image(){
pcfg=$1
if [ -f images/img.tar ] ; then
    printf "******  sdk_44 $pcfg build PASSED  *********\n"
    printf "******  sdk_44 $pcfg build PASSED  *********\n"
else
    printf "[Error]*****  sdk_44 $pcfg build FAILED  **************\n"
    printf "[Error]*****  sdk_44 $pcfg build FAILED  **************\n"
    exit 1
fi
#delete for next check
#rm -rf images
}

if [ $# -lt 5 ]; then
	usage
	exit 1
fi

if [ "${1}" == "all" ]; then
	echo all
else
	if [ "${1}" == "custom" ]; then
		echo custom 
	else
		if [ "${1}" == "both" ]; then
			echo both
		else
			usage
			exit 1
		fi
	fi
fi

if [ "${2}" == "rg" ]; then
	echo rg
else
	if [ "${2}" == "rtk" ]; then
		echo rtk
	else
		if [ "${2}" == "ca8279" ]; then
			echo ca8279
		else
			usage
			exit 1
		fi
	fi
fi

cp ./customized_release_build.lst ../../
cp ./openCustomized.lst ../../
cd ../../
grep -v "^#" ./customized_release_build.lst > tmplst
while read line;do
if [ "$line" == "" ];then
continue 
fi
cd ${BUILD_OMCI_DIR}
modVersion $3 $4 $5

cd ${BUILD_SDK_DIR}
#clean images before build
rm -rf images

#make preconfig
printf "==============================================================\n"
printf "[$(date +"%Y%m%d%H%M")] sdk_44 Release Build $line \n" 
printf "==============================================================\n"
if [ ! -d vendors/Realtek/luna/conf44/$line ];then
   printf "[Error]*****  sdk_44 preconfig44_$line is not existing  **************\n"
   printf "[Error]*****  sdk_44 preconfig44_$line is not existing  **************\n" >> ./build.log
   exit 1
fi

# make clean  >/dev/null  2>&1
make preconfig44_$line
yes "x" | make menuconfig_phase1 > /dev/null 2>&1
yes "x" | make linux_menuconfig > /dev/null 2>&1
yes "x" | make config_menuconfig > /dev/null 2>&1
yes "x" | make menuconfig_phase3 > /dev/null 2>&1
config/setconfig final > /dev/null 2>&1
make all
printf "==============================================================\n"
printf "[$(date +"%Y%m%d%H%M")] sdk_44 Release Build $line Done \n" 
printf " Keep Binary or Open source code \n"
printf "==============================================================\n"

#make sure build successfully
chk_image $line

#get omci dictory name
grep -q "CONFIG_KERNEL_2_6_30=y" .config
if [  $? -eq 0 ]
then
    NEW_OMCI_DIR=$(grep "CONFIG_RSDK_DIR=" .config | awk -F/ '{print $NF}' | sed 's/\///g' | awk -F- '{print "omci_v1_2_6_30-"$3"-"$6}')
    OMCI_PATH="linux-2.6.x/drivers/net/rtl86900/sdk/src/app"
else
    grep -q "CONFIG_KERNEL_3_18_x=y" .config
    if [  $? -eq 0 ]
    then
        grep -q "CONFIG_SMP=y" linux-3.18.x/.config
        if [  $? -eq 0 ]
        then
            NEW_OMCI_DIR=$(grep "CONFIG_RSDK_DIR=" .config | awk -F/ '{print $NF}' | sed 's/\///g' | awk -F- '{print "omci_v1_3_18-"$3"-"$6}')
        else
            NEW_OMCI_DIR=$(grep "CONFIG_RSDK_DIR=" .config | awk -F/ '{print $NF}' | sed 's/\///g' | awk -F- '{print "omci_v1_3_18-"$3"-"$6"-no-smp"}')
        fi
        OMCI_PATH="linux-3.18.x/drivers/net/ethernet/realtek/rtl86900/sdk/src/app"
    else
        grep -q "CONFIG_KERNEL_4_4_x=y" .config
        if [  $? -eq 0 ]
        then
            NEW_OMCI_DIR=$(grep "CONFIG_RSDK_DIR=" .config | awk -F/ '{print $NF}' | sed 's/\///g' | awk -F- '{print "omci_v1_4_4-"$3"-"$6}')
            #NEW_OMCI_DIR="omci_v1_4_4"
            OMCI_PATH="linux-4.4.x/drivers/net/ethernet/realtek/rtl86900/sdk/src/app"
        else
            echo Linux not configure CONFIG_KERNEL_2_6_30/CONFIG_KERNEL_3_18/CONFIG_KERNEL_4_4_x
            exit 1
        fi
    fi
fi
echo NEW_OMCI_DIR : ${NEW_OMCI_DIR}
echo OMCI_PATH : ${OMCI_PATH}

#remove unnessary moidule
cd ${BUILD_SDK_DIR}/${OMCI_PATH}
if  [ "${2}" == "rg" ]; then
rm -rf ${BUILD_OMCI_DIR}/${DRV_CA8279_DIR}
rm -rf ${BUILD_OMCI_DIR}/${DRV_RTK_DIR}
fi
if  [ "${2}" == "rtk" ]; then
rm -rf ${BUILD_OMCI_DIR}/${DRV_FC_DIR}
rm -rf ${BUILD_OMCI_DIR}/${DRV_CA8279_DIR}
rm -rf ${BUILD_OMCI_DIR}/${DRV_RG_DIR}
fi
if  [ "${2}" == "ca8279" ]; then
rm -rf ${BUILD_OMCI_DIR}/${DRV_FC_DIR}
rm -rf ${BUILD_OMCI_DIR}/${DRV_RG_DIR}
rm -rf ${BUILD_OMCI_DIR}/${DRV_RTK_DIR}
fi

if [ "${1}" == "all" ]; then
echo all
all2bin $3 $4 $5
cd ${BUILD_SDK_DIR}/${OMCI_PATH}
mv ${BUILD_OMCI_DIR} ${BUILD_SDK_DIR}/${OMCI_PATH}/${NEW_OMCI_DIR}
else
if [ "${1}" == "custom" ]; then
echo custom 
cus2bin $3 $4 $5
cd ${BUILD_SDK_DIR}/${OMCI_PATH}
mv ${BUILD_OMCI_DIR} ${BUILD_SDK_DIR}/${OMCI_PATH}/${NEW_OMCI_DIR}
else
echo all
cd ${BUILD_SDK_DIR}/${OMCI_PATH}
cp -nr ${BUILD_OMCI_DIR} ${BUILD_OMCI_DIR}-backup

all2bin $3 $4 $5
cd ${BUILD_SDK_DIR}/${OMCI_PATH}
mv ${BUILD_OMCI_DIR} ${BUILD_SDK_DIR}/${OMCI_PATH}/${NEW_OMCI_DIR}

mv ${BUILD_OMCI_DIR}-backup ${BUILD_OMCI_DIR}

cus2bin $3 $4 $5
cd ${BUILD_SDK_DIR}/${OMCI_PATH}
mv ${BUILD_OMCI_DIR} ${BUILD_SDK_DIR}/${OMCI_PATH}/${NEW_OMCI_DIR}-src


fi
fi

svn up
done < tmplst

#remove omci dictory
rm -rf ${BUILD_OMCI_DIR}
rm -rf openCustomized.lst
rm -rf customized_release_build.lst
rm -rf tmplst
rm -rf omci_v1




