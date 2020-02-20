#!/bin/env bash
set -ex

export CC=clang
export CXX=clang++

build_type="release"
target="all"
run_test_name=""
build_id=""
build_dir="build"
clean_build_dir=0
verbose=""
build_share="OFF"
while getopts "b:m:n:t:scv" opts; do
    case $opts in
    b) build_type=$OPTARG ;;
    m) target=$OPTARG ;;
    n) build_id=$OPTARG ;;
    t) run_test_name=$OPTARG ;;
    s) build_share="ON" ;;
    c) clean_build_dir=1 ;;
    v) verbose="-V" ;;
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

mkdir -p $build_dir && cd $build_dir
cmake $project_dir -DCMAKE_BUILD_TYPE=$build_type -DBUILD_SHARED_LIBS=$build_share -DSeastar_EXPERIMENTAL_COROUTINES_TS=ON
make -j $NUM_CORES $target

if [ ! -z "$run_test_name" -a "$run_test_name" != " " ]; then
    if [ $run_test_name == "all" ]; then
        ctest -j $NUM_CORES $verbose
    else
        ctest -j $NUM_CORES $verbose -R $run_test_name
    fi
fi
