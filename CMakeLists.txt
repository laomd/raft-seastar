cmake_minimum_required(VERSION 3.11)
project(raft VERSION 0.1)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

option(MANAGE_DEPS "Use smf's reproducible build" OFF)

if(MANAGE_DEPS)
  message(STATUS "building deps")
  # don't export() the contents to registry
  set(CMAKE_EXPORT_NO_PACKAGE_REGISTRY ON  CACHE "" INTERNAL FORCE)
  # disable system level registry /usr/local/share/cmake/*
  set(CMAKE_FIND_PACKAGE_NO_SYSTEM_PACKAGE_REGISTRY ON  CACHE "" INTERNAL FORCE)
  # disable user package registry ~/.cmake/*
  set(CMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY ON  CACHE "" INTERNAL FORCE)
  # fetch and build dependencies. each dependency is built in isolation allowing
  # tight control over the build configuration, and avoiding any inadvertant cmake
  # cache pollution. the dependencies are built at configure-time and then
  # imported as normal cmake dependencies, which will easily enable future
  # iterations to selectively satisfy dependencies from system packages.
  set(DEPS_BUILD_DIR ${PROJECT_SOURCE_DIR}/deps-build/type.${CMAKE_BUILD_TYPE})
  set(DEPS_INSTALL_DIR ${PROJECT_SOURCE_DIR}/deps-install/type.${CMAKE_BUILD_TYPE})
  configure_file(CMakeLists.txt.in ${DEPS_BUILD_DIR}/CMakeLists.txt @ONLY)
  execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${DEPS_BUILD_DIR})
  if(result)
    message(FATAL_ERROR "CMake step for deps-build failed: ${result}")
  endif()
  execute_process(COMMAND ${CMAKE_COMMAND} --build .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${DEPS_BUILD_DIR})
  if(result)
    message(FATAL_ERROR "Build step for deps-build failed: ${result}")
  endif()
  # wire up dependency search paths.
  list(APPEND CMAKE_PREFIX_PATH "${DEPS_INSTALL_DIR}")
endif()

find_package(Seastar REQUIRED)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/seastar/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

link_libraries(Seastar::seastar)

add_subdirectory(raft)
