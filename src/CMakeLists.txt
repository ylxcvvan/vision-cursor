# ========================= 配置项目各模块构建 =============================
# 在 src 目录下依次构建各模块，生成对应的对象库 Target: VC_<module>
# 注意：模块顺序按依赖关系排列，避免未定义目标被提前引用。
set(MODULES
    common
    log
    algorithm
    vision
    device
    mediapipe
    utils
    interaction
    config
    mapping
    core
    ui
)

set(VC_INTERNAL_TARGETS "")
set(VC_INTERNAL_OBJECTS "")

foreach(MOD ${MODULES})
    add_subdirectory(${MOD})

    # 动态拼接模块目标名：VC_<module>
    set(TGT_NAME "VC_${MOD}")
    list(APPEND VC_INTERNAL_TARGETS ${TGT_NAME})
    list(APPEND VC_INTERNAL_OBJECTS $<TARGET_OBJECTS:${TGT_NAME}>)
endforeach()

# 统一聚合模块对象与依赖，供主程序/测试直接链接
add_library(VC_AllModules INTERFACE)
target_sources(VC_AllModules INTERFACE ${VC_INTERNAL_OBJECTS})
target_link_libraries(VC_AllModules INTERFACE ${VC_INTERNAL_TARGETS})


# ========================= 配置主程序构建 =============================
set(VC_APP_SOURCES
    main.cpp
    ${CMAKE_SOURCE_DIR}/resources/resources.qrc
)

if(WIN32)
    list(APPEND VC_APP_SOURCES ${CMAKE_SOURCE_DIR}/resources/vision_cursor.rc)
endif()

qt_add_executable(VisionCursor MANUAL_FINALIZATION
    ${VC_APP_SOURCES}
)

target_include_directories(VisionCursor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(VisionCursor PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Multimedia
    Qt${QT_VERSION_MAJOR}::OpenGL
    Qt${QT_VERSION_MAJOR}::OpenGLWidgets
    mediapipe
    opencv
    VC_AllModules
)

set_target_properties(VisionCursor PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    # Windows 下是否启用控制台窗口
    WIN32_EXECUTABLE $<IF:$<BOOL:${VC_ENABLE_CONSOLE_WINDOW}>,FALSE,TRUE>
)

# 安装路径（用于打包/部署）
include(GNUInstallDirs)
install(TARGETS VisionCursor
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# MANUAL_FINALIZATION 对应的 Qt 收尾步骤
qt_finalize_executable(VisionCursor)


# ========================= 配置运行期依赖复制 =============================
# 复制 mediapipe 动态库
add_custom_command(TARGET VisionCursor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${MEDIAPIPE_DIR}/bin/mediapipe.dll
        $<TARGET_FILE_DIR:VisionCursor>
)

# 复制 mediapipe 运行数据目录
add_custom_command(TARGET VisionCursor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${MEDIAPIPE_DIR}/training_data
        $<TARGET_FILE_DIR:VisionCursor>/training_data
)

# 复制 OpenCV 运行期 dll
add_custom_command(TARGET VisionCursor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${OPENCV_DIR}/bin
        $<TARGET_FILE_DIR:VisionCursor>
)

# Release 下可选部署 Qt 运行库
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_custom_command(TARGET VisionCursor POST_BUILD
        COMMAND Qt${QT_VERSION_MAJOR}::windeployqt --no-translations
                $<TARGET_FILE:VisionCursor>
    )
endif()
