# tests/common/CMakeLists.txt

add_library(TestCommon STATIC
    vision_cursor_test_fixture.cpp
    vision_cursor_test_fixture.h
    test_helpers.cpp
    test_helpers.h
)

# 1. expose headers
# note: tests include this directory directly.
target_include_directories(TestCommon PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 2. let fixture/helpers access project headers
target_include_directories(TestCommon PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)

# 3. dependencies
target_link_libraries(TestCommon PUBLIC
    gtest_main
    gtest
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Core
    mediapipe
    opencv
    VC_AllModules
)
