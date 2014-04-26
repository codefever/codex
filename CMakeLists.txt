
cmake_minimum_required(VERSION 2.8.8)

set(LIBS glog gflags protobuf)

include_directories(
   "${PROJECT_SOURCE_DIR}"
   "${PROJECT_BINARY_DIR}"
)

# SVN checkout gtest
#add_subdirectory(gtest)

# codex
add_executable(codex codex.cc)
target_link_libraries(codex ${LIBS})

# test

# Add protobuf compilation support
include("FindProtobuf")
find_package(Protobuf REQUIRED)
# Build protobuf
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS mock.proto)
# Build mock_writer
add_executable(mock_writer mock_writer.cc ${PROTO_SRCS})
target_link_libraries(mock_writer ${LIBS})
