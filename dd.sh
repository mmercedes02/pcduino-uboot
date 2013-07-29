#!/bin/bash
# 

if [ -z $1 ]
then
	echo "Usage: $0 /dev/sdX"
	exit 127
fi

if [ $UID -ne 0 ]
then
	echo "need root permission"
	exit 1
fi

dd if=/dev/zero of=$1 bs=1K seek=32 count=992 
dd if=u-boot.bin of=$1 bs=1K seek=32
