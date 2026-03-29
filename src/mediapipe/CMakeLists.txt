file(GLOB_RECURSE MEDIAPIPE_SOURCES *.cpp *.h *.hpp)

add_library(VC_mediapipe OBJECT ${MEDIAPIPE_SOURCES})

target_include_directories(VC_mediapipe PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(VC_mediapipe PUBLIC
    mediapipe
    VC_common
    VC_log
)