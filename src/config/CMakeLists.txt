file(GLOB_RECURSE CONFIG_SOURCES *.cpp *.h *.hpp)

add_library(VC_config OBJECT ${CONFIG_SOURCES})

target_include_directories(VC_config PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(VC_config PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    VC_common
    VC_utils
    VC_interaction
    VC_log
)
