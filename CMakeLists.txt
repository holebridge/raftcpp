cmake_minimum_required (VERSION 3.8)

project ("raftcpp")

set(VCPKG_ROOT "D:\\soft\\vcpkg")

set (PROTOC_PATH "${VCPKG_ROOT}\\installed\\x64-windows\\tools\\protobuf\\protoc.exe")
set (GRPC_PROTOC_PLUGIN_PATH "${VCPKG_ROOT}\\installed\\x64-windows\\tools\\grpc/grpc_cpp_plugin.exe")

execute_process(
    COMMAND ${PROTOC_PATH} -I ${PROJECT_SOURCE_DIR}/src/pb --cpp_out=${PROJECT_SOURCE_DIR}/src/pb ${PROJECT_SOURCE_DIR}/src/pb/raftcpp.proto
    COMMAND_ECHO STDERR
)

execute_process(
    COMMAND ${PROTOC_PATH} -I ${PROJECT_SOURCE_DIR}/src/pb --grpc_out=${PROJECT_SOURCE_DIR}/src/pb --plugin=protoc-gen-grpc=${GRPC_PROTOC_PLUGIN_PATH} ${PROJECT_SOURCE_DIR}/src/pb/raftcpp.proto
    COMMAND_ECHO STDERR
)

set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}\\scripts\\buildsystems\\vcpkg.cmake")

include_directories (${VCPKG_ROOT}/installed/x64-windows/include)
link_directories(${VCPKG_ROOT}/installed/x64-windows/lib)
link_directories(${VCPKG_ROOT}/installed/x64-windows/bin)

list (APPEND CMAKE_PREFIX_PATH "${VCPKG_ROOT}\\installed\\x64-windows")

find_package(protobuf CONFIG REQUIRED)

find_package(gRPC CONFIG REQUIRED)

include_directories(${gRPC_INCLUDE_DIRS})
link_directories(${gRPC_LIBRARY_DIRS})

file(GLOB_RECURSE PROJECT_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc)
file(GLOB_RECURSE PROJECT_HEADER_FILES  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)

add_executable (${PROJECT_NAME} ${PROJECT_SOURCE_FILES} ${PROJECT_HEADER_FILES})

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC
    protobuf::libprotoc protobuf::libprotobuf protobuf::libprotobuf-lite
	gRPC::grpc gRPC::grpc++
)