# Laid out like gRPC's examples/cpp/helloworld: generate the C++ stubs from
# protos/helloworld.proto with protoc + grpc_cpp_plugin at build time, then
# build the server against them.
cmake_minimum_required(VERSION 3.16)

project(app VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(Protobuf CONFIG QUIET)
if(NOT Protobuf_FOUND)
  find_package(Protobuf REQUIRED)
endif()
find_package(gRPC CONFIG REQUIRED)

find_program(PROTOC protoc REQUIRED)
find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)

set(PROTO_FILE "${CMAKE_CURRENT_SOURCE_DIR}/protos/helloworld.proto")
set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${GEN_DIR}")
set(GEN_SRCS
  "${GEN_DIR}/helloworld.pb.cc" "${GEN_DIR}/helloworld.pb.h"
  "${GEN_DIR}/helloworld.grpc.pb.cc" "${GEN_DIR}/helloworld.grpc.pb.h")
add_custom_command(
  OUTPUT ${GEN_SRCS}
  COMMAND ${PROTOC}
  ARGS --grpc_out "${GEN_DIR}" --cpp_out "${GEN_DIR}"
       -I "${CMAKE_CURRENT_SOURCE_DIR}/protos"
       --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
       "${PROTO_FILE}"
  DEPENDS "${PROTO_FILE}")

add_library(hw_grpc_proto ${GEN_SRCS})
target_include_directories(hw_grpc_proto PUBLIC "${GEN_DIR}")
target_link_libraries(hw_grpc_proto PUBLIC gRPC::grpc++ protobuf::libprotobuf)

add_executable(app src/greeter_server.cc)
target_link_libraries(app PRIVATE hw_grpc_proto gRPC::grpc++ gRPC::grpc++_reflection Threads::Threads)

install(TARGETS app RUNTIME DESTINATION bin)
