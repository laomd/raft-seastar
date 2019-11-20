#!/usr/bin/env bash
# build smf

set -ex

dnf install clang git wget gdb clang-tools-extra vim -y
mv /usr/bin/c++ /usr/bin/c++-g++
ln -s /usr/bin/clang++ /usr/bin/c++

cmake_cmd="cmake .. -DCMAKE_BUILD_TYPE=Release"
NUM_CORES=$(cat /proc/cpuinfo | grep "processor" | wc -l)
make_cmd="make install -j $NUM_CORES"

dowload_extract() {
  wget -c $1 -O - | tar -xz --one-top-level=$2 --strip-components 1
}

build_seastar() {
  git clone https://github.com/scylladb/seastar
  cd seastar
  ./install-dependencies.sh
  mkdir build
  cd build
  ${cmake_cmd} \
    -DSeastar_INSTALL=ON \
    -DSeastar_DPDK=OFF \
    -DSeastar_APPS=OFF \
    -DSeastar_DEMOS=OFF \
    -DSeastar_DOCS=OFF \
    -DSeastar_TESTING=OFF \
    -DSeastar_CXX_FLAGS=-Wno-error \
    -DSeastar_LD_FLAGS=-pthread \
    -DSeastar_STD_OPTIONAL_VARIANT_STRINGVIEW=ON \
    -DSeastar_CXX_DIALECT=c++17 \
    -DSeastar_EXPERIMENTAL_COROUTINES_TS=ON \
    -DSeastar_UNUSED_RESULT_ERROR=ON
  ${make_cmd}
  cd ../../
  rm seastar -rf
}

build_zstd() {
  dir=`pwd`
  dowload_extract https://github.com/facebook/zstd/archive/470344d33e1d52a2ada75d278466da8d4ee2faf6.tar.gz zstd
  cd zstd/build/cmake
  mkdir build && cd build
  ${cmake_cmd} -DZSTD_MULTITHREAD_SUPPORT=OFF \
    -DZSTD_LEGACY_SUPPORT=OFF \
    -DZSTD_BUILD_STATIC=ON \
    -DZSTD_BUILD_SHARED=OFF \
    -DZSTD_BUILD_PROGRAMS=OFF
  ${make_cmd}
  cd $dir
}

build_HdrHistogram() {
  dir=`pwd`
  dowload_extract https://github.com/HdrHistogram/HdrHistogram_c/archive/7381c1bf78d462917cf31b736d5d9e700bfedb0f.tar.gz HdrHistogram
  cd HdrHistogram
  mkdir build && cd build
  ${cmake_cmd} -DHDR_HISTOGRAM_BUILD_PROGRAMS=OFF \
    -DHDR_HISTOGRAM_BUILD_SHARED=OFF
  ${make_cmd}
  cd $dir
}

build_xxhash() {
  dir=`pwd`
  dowload_extract  https://github.com/Cyan4973/xxHash/archive/c9970b8ec4155e789f1c3682da923869a496ba9d.tar.gz xxhash
  cd xxhash/cmake_unofficial
  mkdir build && cd build
  ${cmake_cmd} -DBUILD_XXHSUM=OFF \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_ENABLE_INLINE_API=ON
  ${make_cmd}
  cd $dir
}

build_gtest() {
  dir=`pwd`
  dowload_extract  https://github.com/google/googletest/archive/644319b9f06f6ca9bf69fe791be399061044bc3d.tar.gz gtest
  cd gtest
  mkdir build && cd build
  ${cmake_cmd}
  ${make_cmd}
  cd $dir
}

build_gflags() {
  dir=`pwd`
  dowload_extract https://github.com/gflags/gflags/archive/1005485222e8b0feff822c5723ddcaa5abadc01a.tar.gz gflags
  cd gflags
  mkdir build && cd build
  ${cmake_cmd} -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_STATIC_LIBS=ON \
    -DBUILD_gflags_LIB=ON \
    -DBUILD_TESTING=OFF \
    -DBUILD_PACKAGING=OFF
  ${make_cmd}
  cd $dir
}

build_glog() {
  dir=`pwd`
  dowload_extract  https://github.com/google/glog/archive/5d46e1bcfc92bf06a9ca3b3f1c5bb1dc024d9ecd.tar.gz glog
  cd glog
  mkdir build && cd build
  ${cmake_cmd} -DBUILD_SHARED_LIBS=OFF \
    -DWITH_GFLAGS=ON
  ${make_cmd}
  cd $dir
}

build_googlebenchmark() {
  dir=`pwd`
  dowload_extract  https://github.com/google/benchmark/archive/eec9a8e4976a397988c15e5a4b71a542375a2240.tar.gz googlebenchmark
  cd googlebenchmark
  mkdir build && cd build
  ${cmake_cmd} -DBENCHMARK_ENABLE_GTEST_TESTS=OFF \
    -DBENCHMARK_ENABLE_TESTING=OFF
  ${make_cmd}
  cd $dir
}

build_flatbuffers() {
  dir=`pwd`
  dowload_extract  https://github.com/google/flatbuffers/archive/bc7ede8fb3a754a04e507992d5514b528270ee06.tar.gz flatbuffers
  cd flatbuffers
  mkdir build && cd build
  ${cmake_cmd} -DFLATBUFFERS_INSTALL=ON \
    -DFLATBUFFERS_BUILD_FLATC=ON \
    -DFLATBUFFERS_BUILD_TESTS=OFF \
    -DFLATBUFFERS_BUILD_FLATHASH=OFF
  ${make_cmd}
  cd $dir
}

smf/tools/docker-deps.sh
smf/install-deps.sh
mkdir -p deps-build
cd deps-build
build_seastar
build_zstd
build_HdrHistogram
build_xxhash
build_gtest
build_gflags
build_glog
build_googlebenchmark
build_flatbuffers
cd ..
