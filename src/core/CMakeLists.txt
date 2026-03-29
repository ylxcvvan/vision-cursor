file(GLOB_RECURSE CORE_SOURCES *.cpp *.h *.hpp)

add_library(VC_core OBJECT ${CORE_SOURCES})

target_include_directories(VC_core PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(VC_core PUBLIC
    VC_common
    VC_config
    VC_interaction
    VC_mapping
    VC_vision
    VC_mediapipe
    VC_log
)
