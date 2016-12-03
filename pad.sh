#!/bin/sh

len=$(wc -c $1 | cut -d ' ' -f 1)
echo "$1 size: $len"
dd if=/dev/zero of=$1 oflag=append conv=notrunc bs=$((65536-len)) count=1
