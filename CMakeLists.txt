cmake_minimum_required(VERSION 3.10)

project(zuckerli)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 添加 abseil-cpp 作为子目录
add_subdirectory(abseil-cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
# aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src SRC_LIST)
# 下面这种方式可以用通配符指定
file(GLOB SRC_LIST ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc)

# 添加 zuckerli 动态库
#add_library(zuckerli STATIC ${SRC_LIST} src/zuckerli.cc) # 可以修改为SHARED
#target_include_directories(zuckerli PUBLIC ${PROJECT_SOURCE_DIR}/include)
#target_link_libraries(zuckerli absl::flags absl::flags_parse)

# 查找gflags库
find_package(gflags REQUIRED)
#include_directories(${gflags_INCLUDE_DIRS})
#link_directories(${gflags_LIBRARY_DIRS})

# 添加可执行程序示例
add_executable(app main/sample_main.cc ${SRC_LIST})
target_include_directories(app PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(app absl::flags absl::flags_parse)

add_executable(encode main/encode_main.cc ${SRC_LIST})
target_include_directories(encode PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(encode absl::flags absl::flags_parse)

add_executable(benchmark_get_successors test/benchmark_get_successors.cpp ${SRC_LIST})
target_include_directories(benchmark_get_successors PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(benchmark_get_successors absl::flags absl::flags_parse)

add_executable(benchmark_sample test/benchmark_sample.cpp ${SRC_LIST})
target_include_directories(benchmark_sample PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(benchmark_sample absl::flags absl::flags_parse)

add_executable(test main/test.cpp)
# 将gflags库包含到你的项目中
target_include_directories(test PUBLIC ${gflags_INCLUDE_DIRS})
target_link_libraries(test ${GFLAGS_LIBRARIES})