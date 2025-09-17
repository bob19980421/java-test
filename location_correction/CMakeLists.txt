# CMake 最低版本要求
cmake_minimum_required(VERSION 3.10)

# 项目名称
project(location_correction VERSION 1.0.0 LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
if(MSVC)
    # Windows 平台下的编译选项
    add_compile_options(/W4 /WX /EHsc)
else()
    # Linux/macOS 平台下的编译选项
    add_compile_options(-Wall -Wextra -Werror -pedantic)
endif()

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 包含头文件目录
include_directories(include)

# 收集源文件
file(GLOB_RECURSE MODEL_SOURCES "src/model/*.cpp")
file(GLOB_RECURSE UTIL_SOURCES "src/util/*.cpp")
file(GLOB_RECURSE DATA_SOURCES "src/data/*.cpp")
file(GLOB_RECURSE ALGORITHM_SOURCES "src/algorithm/*.cpp")
file(GLOB_RECURSE SERVICE_SOURCES "src/service/*.cpp")
file(GLOB_RECURSE MAIN_SOURCE "src/main.cpp")

# 合并所有源文件
set(SOURCES
    ${MODEL_SOURCES}
    ${UTIL_SOURCES}
    ${DATA_SOURCES}
    ${ALGORITHM_SOURCES}
    ${SERVICE_SOURCES}
    ${MAIN_SOURCE}
)

# 创建可执行文件
add_executable(location_correction ${SOURCES})

# 添加依赖库
if(WIN32)
    # Windows 平台特定的库
    target_link_libraries(location_correction PRIVATE ws2_32)
elseif(UNIX)
    # Linux/macOS 平台特定的库
    target_link_libraries(location_correction PRIVATE pthread)
endif()

# 安装配置
install(TARGETS location_correction DESTINATION bin)

# 启用测试
option(BUILD_TESTS "Build tests" ON)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

# 打印构建信息
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Output Directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")