file(GLOB UI_SOURCES
    *.cpp
    *.h
    *.ui
)

add_library(VC_ui OBJECT ${UI_SOURCES})

# 配置该模块的头文件路径：PUBLIC 属性允许调用者自动获得路径
target_include_directories(VC_ui PUBLIC 
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(VC_ui PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::OpenGL
    Qt${QT_VERSION_MAJOR}::OpenGLWidgets
    VC_common
    VC_core
    VC_config
    VC_utils
)
