file(GLOB_RECURSE INTERACTION_SOURCES *.cpp *.h *.hpp)

add_library(VC_interaction OBJECT ${INTERACTION_SOURCES})

target_include_directories(VC_interaction PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(VC_interaction PUBLIC
    VC_common
    VC_utils
    VC_algorithm
    VC_log
)
