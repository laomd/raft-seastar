cmake_minimum_required(VERSION 3.7.0)
project(SMF VERSION "0.1.0" LANGUAGES CXX C)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -v")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os")
if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Seastar REQUIRED)
find_package(Flatbuffers REQUIRED)
find_package(gflags REQUIRED)
find_package(glog REQUIRED)
find_package(Boost COMPONENTS
  system
  filesystem
  REQUIRED)
find_package (Threads)
find_package(Hdrhistogram REQUIRED)
find_package(xxHash REQUIRED)
find_package(Zstd REQUIRED)

# put all the libs and binaries in one place
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(smfc_generator)
include(tests)
include(ccache)

add_subdirectory(src)