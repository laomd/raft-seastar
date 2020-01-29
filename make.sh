#!/bin/env bash
set -ex

export CC=clang
export CXX=clang++

build_type="release"
target="all"
run_test=0
build_id=""
build_dir="build"
clean_build_dir=0
verbose=0
build_share=0
while getopts "b:t:m:n:s:c:v:" opts; do
    case $opts in
    b) build_type=$OPTARG ;;
    m) target=$OPTARG ;;
    t) run_test=$OPTARG ;;
    n) build_id=$OPTARG ;;
    d) build_share=$OPTARG ;;
    c) clean_build_dir=$OPTARG ;;
    v) verbose=$OPTARG ;;
    ?) ;;
    esac
done

build_type=$(echo $build_type | tr [A-Z] [a-z])
if [ "$build_type" == "release" ]; then
    build_type="Release"
elif [ "$build_type" == "debug" ]; then
    build_type="Debug"
else
    echo "invalid build type"
    exit 1
fi
NUM_CORES=$(cat /proc/cpuinfo | grep "processor" | wc -l)
echo "-- build with $NUM_CORES cores, type: $build_type, target: $target"

project_dir="$(
    cd "$(dirname "$0")"
    pwd -P
)"
build_dir="$build_dir$build_id/$build_type"
if [ $clean_build_dir == 1 ]; then
    rm -rf $build_dir
fi
if [ $build_share == 1 ]; then
    build_share=ON
else
    build_share=OFF
fi

mkdir -p $build_dir && cd $build_dir
cmake $project_dir -DCMAKE_BUILD_TYPE=$build_type -DBUILD_SHARED_LIBS=$build_share
make -j $NUM_CORES $target

if [ $run_test == 1 ]; then
    if [ $verbose == 1 ]; then
        ctest -j $NUM_CORES -V
    else
        ctest -j $NUM_CORES
    fi
fi
