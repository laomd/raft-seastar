#! /bin/bash
set -e

addrs=$(cat $2 | awk '{ print $2 }')
echo $addrs
addr2line -e $1 $addrs
