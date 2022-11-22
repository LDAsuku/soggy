# CMakeLists.txt

cmake_minimum_required(VERSION 3.13)

project(soggy)

option(SOGGY_PROTO_SHARED "Use a shared library for protos. Decreases build time but doesn't work on some systems." OFF)

# debug build by default
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Debug")
endif()

find_package(Protobuf REQUIRED)
find_package(Lua REQUIRED)

add_subdirectory("extern/enet-1.3.17")
add_subdirectory("extern/replxx-0.0.4")

set(CMAKE_CXX_STANDARD "17")

# protos
if(SOGGY_PROTO_SHARED)
	add_library(soggy_proto SHARED)
else()
	add_library(soggy_proto STATIC)
endif()
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/proto/proto")
file(GLOB proto_srcs "${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto")
target_include_directories(soggy_proto PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/proto")
target_include_directories(soggy_proto PUBLIC "${Protobuf_INCLUDE_DIRS}")
foreach(f ${proto_srcs})
	protobuf_generate_cpp(proto_src proto_header ${f} PROTOC_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/proto/proto")
	target_sources(soggy_proto PRIVATE ${proto_src})
endforeach()
target_link_libraries(soggy_proto PUBLIC ${Protobuf_LIBRARY})

# cmdids processor
add_executable(soggy_cmdids_processor "tool/cmdids_processor.cpp")
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
target_include_directories(soggy PRIVATE "${LUA_INCLUDE_DIR}")
target_include_directories(soggy PRIVATE "extern/enet/1.3.17/include")
if(WIN32)
	target_link_libraries(soggy PRIVATE winmm ws2_32)
endif()
target_link_libraries(soggy PRIVATE soggy_proto)
target_link_libraries(soggy PRIVATE enet)
target_link_libraries(soggy PRIVATE replxx)
target_link_libraries(soggy PRIVATE ${LUA_LIBRARIES})
