file(GLOB_RECURSE MAPPING_SOURCES *.cpp *.h *.hpp)

add_library(VC_mapping OBJECT ${MAPPING_SOURCES})

target_include_directories(VC_mapping PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(VC_mapping PUBLIC
    VC_common
    VC_interaction
    VC_device
    VC_log
)
