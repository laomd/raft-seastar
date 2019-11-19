#! /bin/bash

set -e

for port in 12000 12001 12002 12003 12004; do
  args="-p $port"
  for other in 12000 12001 12002 12003 12004; do
    if [ $other != $port ]; then
      args="$args -o 127.0.0.1:$other"
    fi
  done
  cmd="./test_raft -c 1 $args"
  echo $cmd
  nohup $cmd > $port.log 2>1 &
done
wait