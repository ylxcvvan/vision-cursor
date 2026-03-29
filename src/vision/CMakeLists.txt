file(GLOB_RECURSE VISION_SOURCES *.cpp *.h *.hpp)

add_library(VC_vision OBJECT ${VISION_SOURCES})

target_include_directories(VC_vision PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(VC_vision PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Multimedia
    opencv
    VC_common
    VC_log
)