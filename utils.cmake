execute_process(COMMAND which grpc_cpp_plugin OUTPUT_VARIABLE GRPC_CPP_PLUGIN)
string(STRIP ${GRPC_CPP_PLUGIN} GRPC_CPP_PLUGIN)
macro(PROTOBUF_GENERATE_CPP2 dir)
    set(PROTO_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${dir})
    set(PROTO_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/${dir})
    file(MAKE_DIRECTORY ${PROTO_OUT_DIR})

    file(GLOB PROTO_LIST ${PROTO_PATH}/*.proto)
    foreach(proto ${PROTO_LIST})
        execute_process(COMMAND ${PROTOBUF_PROTOC_EXECUTABLE} --proto_path=${PROTO_PATH} --cpp_out=${PROTO_OUT_DIR} ${proto})
    endforeach(proto)
    file(GLOB PROTO_HDRS ${PROTO_OUT_DIR}/*.pb.h)
    file(GLOB PROTO_SRCS ${PROTO_OUT_DIR}/*.pb.cc)
endmacro(PROTOBUF_GENERATE_CPP2)

macro(PROTOBUF_GENERATE_GRPC dir)
    set(PROTO_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${dir})
    set(PROTO_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/${dir})
    file(MAKE_DIRECTORY ${PROTO_OUT_DIR})

    file(GLOB PROTO_LIST ${PROTO_PATH}/*.proto)
    foreach(proto ${PROTO_LIST})
        execute_process(COMMAND ${PROTOBUF_PROTOC_EXECUTABLE} 
            --proto_path=${PROTO_PATH} 
            --grpc_out=${PROTO_OUT_DIR}
            --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
            ${proto})
    endforeach(proto)
    file(GLOB PROTO_GRPC_HDRS ${PROTO_OUT_DIR}/*.grpc.pb.h)
    file(GLOB PROTO_GRPC_SRCS ${PROTO_OUT_DIR}/*.grpc.pb.cc)
endmacro(PROTOBUF_GENERATE_GRPC)

function(add_tests TEST_SRCS)
    foreach(test_case ${TEST_SRCS})
        get_filename_component(barename ${test_case} NAME)
        string(REGEX REPLACE "\\.[^.]*$" "" target_name ${barename})
        add_executable(${target_name} ${barename})
        add_test(NAME ${target_name} COMMAND ${target_name})
    endforeach(test_case)
endfunction(add_tests)
