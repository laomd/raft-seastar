# remove_definitions("-DBOOST_TEST_DYN_LINK")
add_custom_target (raft_unit_tests)
set (Seastar_UNIT_TEST_SMP
  10
  CACHE
  STRING
  "Run unit tests with this many cores.")

#
# Define a new unit test with the given name.
#
# seastar_add_test (name
#   [KIND {SEASTAR,BOOST,CUSTOM}]
#   [SOURCES source1 source2 ... sourcen]
#   [WORKING_DIRECTORY dir]
#   [LIBRARIES library1 library2 ... libraryn]
#   [RUN_ARGS arg1 arg2 ... argn])
#
# There are three kinds of test we support (the KIND parameter):
#
# - SEASTAR: Unit tests which use macros like `SEASTAR_TEST_CASE`
# - BOOST: Unit tests which use macros like `BOOST_AUTO_TEST_CASE`
# - CUSTOM: Custom tests which need to be specified
#
# SEASTAR and BOOST tests will have their output saved for interpretation by the Jenkins continuous integration service
# if this is configured for the build.
#
# KIND can be omitted, in which case it is assumed to be SEASTAR.
#
# If SOURCES is provided, then the test files are first compiled into an executable which has the same name as the test
# but with a suffix ("_test").
#
# WORKING_DIRECTORY can be optionally provided to choose where the test is executed.
#
# If LIBRARIES is provided along with SOURCES, then the executable is additionally linked with these libraries.
#
# RUN_ARGS are optional additional arguments to pass to the executable. For SEASTAR tests, these come after `--`. For
# CUSTOM tests with no SOURCES, this parameter can be used to specify the executable name as well as its arguments since
# no executable is compiled.
#
function (seastar_add_test name)
  set (test_kinds
    SEASTAR
    BOOST
    CUSTOM)

  cmake_parse_arguments (parsed_args
    ""
    "WORKING_DIRECTORY;KIND"
    "RUN_ARGS;SOURCES;LIBRARIES;DEPENDENCIES"
    ${ARGN})

  if (NOT parsed_args_KIND)
    set (parsed_args_KIND SEASTAR)
  elseif (NOT (parsed_args_KIND IN_LIST test_kinds))
    message (FATAL_ERROR "Invalid test kind. KIND must be one of ${test_kinds}")
  endif ()

  if (parsed_args_SOURCES)
    # Each kind of test must populate the `args` and `libraries` lists.
    set (libraries "${parsed_args_LIBRARIES}")
    set (deps "${}")
    set (args "")

    if (parsed_args_KIND STREQUAL "SEASTAR")
      list (APPEND libraries raft_testing)
      list (APPEND args -- -c ${Seastar_UNIT_TEST_SMP})
      list (APPEND args ${parsed_args_RUN_ARGS})
    elseif (parsed_args_KIND STREQUAL "BOOST")
      set (args "")
      list (APPEND libraries Boost::unit_test_framework)
    endif ()
    list (APPEND args ${parsed_args_RUN_ARGS})

    set (executable_target test_${name}_run)
    add_executable (${executable_target} ${parsed_args_SOURCES})
    target_link_libraries (${executable_target} PRIVATE ${libraries})
    target_compile_definitions (${executable_target} PRIVATE SEASTAR_TESTING_MAIN)
    target_include_directories (${executable_target} PRIVATE include_directories(${CMAKE_CURRENT_BINARY_DIR}))
    if (parsed_args_DEPENDENCIES)
      add_dependencies (${executable_target} ${parsed_args_DEPENDENCIES})
    endif ()
    add_dependencies (raft_unit_tests ${executable_target})
    set (forwarded_args COMMAND ${executable_target} ${args})
  else ()
    message (FATAL_ERROR "SOURCES are required for ${parsed_args_KIND} tests")
  endif ()

  if (parsed_args_WORKING_DIRECTORY)
    list (APPEND forwarded_args WORKING_DIRECTORY ${parsed_args_WORKING_DIRECTORY})
  endif ()

  set (target ${name}.test)
  add_custom_target (${target}
    ${forwarded_args}
    USES_TERMINAL)
  add_test (
    NAME ${name}
    COMMAND ${executable_target})
endfunction ()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set (allocator_test_args --iterations 5)
else ()
  set (allocator_test_args --time 0.1)
endif ()
