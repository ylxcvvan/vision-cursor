file(GLOB_RECURSE UTILS_SOURCES *.cpp *.h *.hpp)

add_library(VC_utils OBJECT ${UTILS_SOURCES})

target_include_directories(VC_utils PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(VC_utils PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    opencv
    mediapipe
    VC_common
    VC_log
    VC_vision
)