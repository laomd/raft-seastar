#! /bin/bash

set -ex

for port in 12000 12001 12002 12003 12004; do
  args="-p $port"
  for other in 12000 12001 12002 12003 12004; do
    if [ $other != $port ]; then
      args="$args -o 127.0.0.1:$other"
    fi
  done
  ./test_raft $args > $port.log &
done
wait