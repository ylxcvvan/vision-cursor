# 确保 GTest 存在
if(NOT TARGET gtest)
    find_package(GTest REQUIRED)
endif()

include(GoogleTest)

# 设置输出上限字数为10MB而不是1KB
file(WRITE "${CMAKE_BINARY_DIR}/CTestCustom.cmake" "
    set(CTEST_CUSTOM_MAXIMUM_PASSED_TEST_OUTPUT_SIZE \"10485760\")
    set(CTEST_CUSTOM_MAXIMUM_FAILED_TEST_OUTPUT_SIZE \"10485760\")
")

add_subdirectory(fixture) # 先编译通用的夹具类

# =============================================================
# 2. 定义一个“超级宏”：自动配置测试目标
# =============================================================
# 这个函数的作用是：你给它一个名字和测试文件，它自动帮你把
# 业务源码、头文件路径、OpenCV依赖、Qt依赖全配好。
# 使用的语法: vision_cursor_add_test_executable(目标名 源文件列表)

function(vision_cursor_add_test_executable target_name test_sources)
    
    # A. 创建可执行文件 (测试代码 + 根目录传来的业务代码)
    add_executable(${target_name} 
        ${test_sources}
    )

    # B. 统一设置头文件路径
    target_include_directories(${target_name} PRIVATE
        ${PROJECT_SOURCE_DIR}/src
    )

    # C. 统一链接依赖 (在这里一处修改，所有测试生效)
    target_link_libraries(${target_name} PRIVATE
        TestCommon
        gtest_main
        gtest
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Multimedia
        mediapipe
        opencv
        VC_AllModules
    )
    
    gtest_discover_tests(${target_name}
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:${target_name}>"
    )

    add_dependencies(${target_name} VisionCursor)

endfunction()

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/test_data" 
     DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

# =============================================================
# 注册所有子模块
# =============================================================
add_subdirectory(integration)
add_subdirectory(unit)
