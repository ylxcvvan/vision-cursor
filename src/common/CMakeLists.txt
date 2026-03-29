file(GLOB_RECURSE COMMON_SOURCES *.cpp *.h *.hpp)

add_library(VC_common OBJECT ${COMMON_SOURCES})

target_include_directories(VC_common PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(VC_common PUBLIC)