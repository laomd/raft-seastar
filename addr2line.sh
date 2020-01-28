#! /bin/bash
set -e

cat $2 | awk '{ print $2 }' | addr2line -e $1
