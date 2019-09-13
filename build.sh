#!/bin/env bash
set -e

build_type=""
tmp=$(echo $1 | tr [A-Z] [a-z])
if [ $# == 0 ] || [ "$tmp" == "release" ]; then
    build_type="Release"
elif [ "$tmp" == "debug" ]; then
    build_type="Debug"
else
    echo "invalid build type"
fi
if [ "$build_type" != "" ]; then
    NUM_CORES=$(cat /proc/cpuinfo | grep "processor" | wc -l)
    build_dir="build/$build_type"
    rm -rf build/* && rm -rf build && mkdir -p $build_dir && cd $build_dir
    cmake ../../ -DCMAKE_BUILD_TYPE=$build_type
    make -j $NUM_CORES
    cd - && mv build /var && ln -s /var/build .
fi