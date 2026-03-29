#========================= 设置项目基础要求 =============================
cmake_minimum_required(VERSION 3.24)

project(VisionCursor VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    foreach(flag_var
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS_MINSIZEREL
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_CXX_FLAGS_MINSIZEREL
    )
    endforeach()

    if(CMAKE_GENERATOR MATCHES "Ninja")
        set(CMAKE_DEPFILE_FLAGS_C "")
        set(CMAKE_DEPFILE_FLAGS_CXX "")
    endif()
endif()


#========================= 设置全局输出目录 =============================
# 这样 src 和 tests 都会自动输出到 build/exe
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")


#========================= 检查并配置三方库 =============================
# ------------------- logging -----------------------
# 这个选项用于设置生成的最后VisionCursor.exe是否持有命令行
option(VC_ENABLE_CONSOLE_WINDOW "Enable Windows console window for runtime logs" OFF)

# ------------------- QT6 ------------------------
if(WIN32)
    # Avoid noisy optional Vulkan detection messages from Qt on machines
    # without Vulkan SDK headers installed.
    set(CMAKE_DISABLE_FIND_PACKAGE_WrapVulkanHeaders TRUE CACHE BOOL "Disable optional Qt Vulkan headers package" FORCE)
    set(CMAKE_DISABLE_FIND_PACKAGE_Vulkan TRUE CACHE BOOL "Disable optional Vulkan package" FORCE)
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Core Multimedia OpenGL OpenGLWidgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Core Multimedia OpenGL OpenGLWidgets)


# ---------------- mediapipe ---------------------

set(MEDIAPIPE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/libmediapipe)
if(NOT EXISTS ${MEDIAPIPE_DIR})
    message(FATAL_ERROR "cmake error: 缺少mediapipe库,请确保3rdparty/libmediapipe目录存在")
endif()

add_library(mediapipe SHARED IMPORTED)

set_target_properties(mediapipe PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES ${MEDIAPIPE_DIR}/include
    IMPORTED_LOCATION ${MEDIAPIPE_DIR}/bin/mediapipe.dll
    IMPORTED_IMPLIB ${MEDIAPIPE_DIR}/lib/mediapipe.lib
)


# ------------------- openCV ----------------------
# 1. 基础路径设置 (请确认路径是否包含 x64/vc16)
set(OPENCV_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/opencv)

# 2. 检查路径是否存在 
if(NOT EXISTS ${OPENCV_DIR})
    message(FATAL_ERROR "CMake Error: 缺少OpenCV库, 请确保路径正确: ${OPENCV_DIR}")
endif()

add_library(opencv SHARED IMPORTED)

set_target_properties(opencv PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${OPENCV_DIR}/include"

    IMPORTED_IMPLIB_DEBUG "${OPENCV_DIR}/lib/opencv_world470d.lib"
    IMPORTED_LOCATION_DEBUG "${OPENCV_DIR}/bin/opencv_world470d.dll"

    IMPORTED_IMPLIB_RELEASE "${OPENCV_DIR}/lib/opencv_world470.lib"
    IMPORTED_LOCATION_RELEASE "${OPENCV_DIR}/bin/opencv_world470.dll"

    IMPORTED_IMPLIB_MINSIZEREL "${OPENCV_DIR}/lib/opencv_world470.lib"
    IMPORTED_LOCATION_MINSIZEREL "${OPENCV_DIR}/bin/opencv_world470.dll"

    IMPORTED_IMPLIB_RELWITHDEBINFO "${OPENCV_DIR}/lib/opencv_world470.lib"
    IMPORTED_LOCATION_RELWITHDEBINFO "${OPENCV_DIR}/bin/opencv_world470.dll"
)

# ------------------- Google的GTest库 ----------------------
# === 1. 引入本地 GTest ===
# 防止 GTest 修改你的编译选项（如 /MD 或 /MT）
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# 这一步会编译 3rdparty/googletest 下的源码
add_subdirectory(3rdparty/googletest)
# 开启 CTest 测试功能
enable_testing()
set(ENV{CTEST_OUTPUT_SIZE_LIMIT} 0)

#========================= 编译项目源代码 =============================
add_subdirectory(src)

option(VC_BUILD_TESTS "Build VisionCursor tests" ON)
if(VC_BUILD_TESTS)
    add_subdirectory(tests)
endif()
