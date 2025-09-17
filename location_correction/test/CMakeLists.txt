# CMake 配置文件 - 测试目录

# 查找 Google Test 框架
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# 设置测试输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/tests)

# 收集测试源文件
file(GLOB_RECURSE TEST_SOURCES "*.cpp")

# 创建测试可执行文件
add_executable(location_correction_tests ${TEST_SOURCES})

# 链接 GTest 库和主程序库
target_link_libraries(location_correction_tests
    PRIVATE
    ${GTEST_BOTH_LIBRARIES}
    location_correction_lib
)

# 添加测试
add_test(NAME all_tests COMMAND location_correction_tests)

# 包含头文件目录
target_include_directories(location_correction_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)