# CMakeLists.txt

cmake_minimum_required(VERSION 3.13)

project(soggy)

# debug build by default
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Debug")
endif()

find_package(PkgConfig REQUIRED)
find_package(Protobuf REQUIRED)


# libenet
pkg_check_modules(libenet IMPORTED_TARGET libenet REQUIRED)
add_library(libenet INTERFACE)
target_compile_options(libenet INTERFACE ${libenet_CFLAGS})
target_link_libraries(libenet INTERFACE ${libenet_LDFLAGS})

# readline
pkg_check_modules(readline IMPORTED_TARGET readline REQUIRED)
add_library(readline INTERFACE)
target_compile_options(readline INTERFACE ${readline_CFLAGS})
target_link_libraries(readline INTERFACE ${readline_LDFLAGS})

# lua53
pkg_check_modules(lua53 IMPORTED_TARGET lua53 REQUIRED)
add_library(lua53 INTERFACE)
target_compile_options(lua53 INTERFACE ${lua53_CFLAGS})
target_link_libraries(lua53 INTERFACE ${lua53_LDFLAGS})

# protos
# use a shared library because linking a 100mb+ static library takes extremely long
# maybe use a static library for release builds?
add_library(soggy_proto SHARED)
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/proto/proto")
file(GLOB proto_srcs "${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto")
target_include_directories(soggy_proto PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/proto")
# target_compile_definitions(soggy_proto PUBLIC "PROTOBUF_NAMESPACE_ID=proto")
foreach(f ${proto_srcs})
	protobuf_generate_cpp(proto_src proto_header ${f} PROTOC_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/proto/proto")
	target_sources(soggy_proto PRIVATE ${proto_src})
endforeach()
target_link_libraries(soggy_proto PUBLIC ${Protobuf_LIBRARY})

# cmdids processor
add_executable(soggy_cmdids_processor "tool/cmdids_processor.cpp")
set_target_properties(soggy_cmdids_processor PROPERTIES CXX_STANDARD "17")
add_custom_command(
	OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/proto/proto/cmdids.cpp" "${CMAKE_CURRENT_BINARY_DIR}/proto/proto/cmdids.hpp"
	DEPENDS soggy_cmdids_processor "${CMAKE_CURRENT_SOURCE_DIR}/proto/cmdids.csv"
	COMMAND soggy_cmdids_processor "${CMAKE_CURRENT_SOURCE_DIR}/proto/cmdids.csv" "${CMAKE_CURRENT_BINARY_DIR}/proto/proto/cmdids.cpp" "${CMAKE_CURRENT_BINARY_DIR}/proto/proto/cmdids.hpp"
)
# cmdids processor output
target_sources(soggy_proto PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/proto/proto/cmdids.cpp")

# soggy
add_executable(soggy
	"src/soggy.cpp"
	"src/packet.cpp"
	"src/game_data.cpp"
	"src/log.cpp"
)
target_include_directories(soggy PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(soggy PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/extern")
set_target_properties(soggy PROPERTIES CXX_STANDARD "17")
target_link_libraries(soggy PRIVATE soggy_proto)
target_link_libraries(soggy PRIVATE libenet)
target_link_libraries(soggy PRIVATE readline)
target_link_libraries(soggy PRIVATE lua)
