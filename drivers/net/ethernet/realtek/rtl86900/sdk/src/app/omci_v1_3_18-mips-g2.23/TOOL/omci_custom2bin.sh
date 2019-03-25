#!/bin/sh

usage()
{
	printf "Usage: \n [custom2bin]:\tomci_custom2bin.sh [YourPreconfig] [sfu|hgu|hgu-fc] [omci version]\n"
}

OMCI_PWD=$(pwd | sed 's/TOOL.*//g')
BUILD_PWD=$(pwd |sed 's/linux.*//g')

function remove_no_release_src {
	echo "keep $1"
	
	if [ "${1}" != "" ]; then
		if [ "${1}" != "hgu" ]; then
			find "${OMCI_PWD}/DRV" -name "*_rg" | xargs -n2 rm -rf && test 0 -ne $? && exit 1
		fi

		if [ "${1}" != "sfu" ]; then
			find "${OMCI_PWD}/DRV" -name "*_rtk" | xargs -n2 rm -rf && test 0 -ne $? && exit 1
		fi

		if [ "${1}" != "hgu-fc" ]; then
			find "${OMCI_PWD}/DRV" -name "*_fc" | xargs -n2 rm -rf && test 0 -ne $? && exit 1
		fi
	fi
}

if [ $# -lt 1 ]; then
	usage
	exit 1
fi

cd ${OMCI_PWD}
echo $(pwd)

#update OMCI customized feature
#svn up (restore .c file)
#svn up $BASE_PWD
if [ "${3}" == "" ]; then
	echo "OMCI Version is refered to SVN revison... "
	OMCI_VERSION=`svn info | grep -e Rev: | sed -e 's/.*Rev: //'`
else
	echo "OMCI VERSION is "${3}""
	OMCI_VERSION="${3}"
fi
cd $BUILD_PWD

make $1 && test 0 -ne $? && exit 1
yes "xx" | make menuconfig
make all && test 0 -ne $? && exit 1

find "${OMCI_PWD}/FAL/src/features" -name "*.c" | xargs -n2 rm && test 0 -ne $? && exit 1
find "${OMCI_PWD}/FAL/src/cfg" -name "*.c" | xargs -n2 rm && test 0 -ne $? && exit 1
find "${OMCI_PWD}/DRV/features" -name "*.c" | xargs -n2 rm && test 0 -ne $? && exit 1

find "${OMCI_PWD}/DRV/features" -name "*.ko" | sed -e p -e "s/ko/ko_${OMCI_VERSION}/g" | xargs -n2 mv && test 0 -ne $? && exit 1
find "${OMCI_PWD}/FAL/src/features" -name "*.so" | sed -e p -e "s/so/so.${OMCI_VERSION}/g" | xargs -n2 mv && test 0 -ne $? && exit 1

make clean && test 0 -ne $? && exit 1

#remove and replace back src for open customized feature.
cd ${OMCI_PWD}
grep -v "^#" ${OMCI_PWD}/TOOL/openCustomized.lst > tmpOpenlst
while read svnline;do
	if [ "$svnline" == "" ];then
		continue 
	fi
	#remove open customized releated
	find "${OMCI_PWD}" -name "*$svnline*" | xargs -n2 rm -rf && test 0 -ne $? && exit 1
	#restore open customized .c file
	cd ${OMCI_PWD}
	svn export http://cadinfo.realtek.com.tw/svn/CN/Switch/PON/private/rtk_omci_v1/omci_v1/DRV/features/$svnline/ ./DRV/features/$svnline/
	svn export http://cadinfo.realtek.com.tw/svn/CN/Switch/PON/private/rtk_omci_v1/omci_v1/FAL/src/features/$svnline.c ./FAL/src/features/$svnline.c
done < tmpOpenlst

remove_no_release_src $2

if [ "${2}" == "" ]; then
	echo "please delete pf_rtk for HGU device, or delete pf_rg for SFU device."
fi

