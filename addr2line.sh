#! /bin/bash
set -e

addrs=$(cat $2 | awk '{ print $1 }')
echo $addrs
addr2line -e $1 $addrs
