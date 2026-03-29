file(GLOB_RECURSE LOG_SOURCES *.cpp *.h *.hpp)

add_library(VC_log OBJECT ${LOG_SOURCES})

target_include_directories(VC_log PUBLIC
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/3rdparty/concurrentqueue
)

target_compile_definitions(VC_log PUBLIC
    VC_LOG_DEFAULT_CONSOLE=$<IF:$<BOOL:${VC_ENABLE_CONSOLE_WINDOW}>,1,0>
)
