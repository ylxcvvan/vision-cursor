set(DEVICE_SOURCES
    mouse_controller.h
)

if(WIN32)
    list(APPEND DEVICE_SOURCES
        platform/mouse_windows.cpp
    )
else()
    message(FATAL_ERROR "VC_device currently supports Windows only.")
endif()

add_library(VC_device OBJECT ${DEVICE_SOURCES})

target_include_directories(VC_device PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)

if(WIN32)
    target_link_libraries(VC_device PUBLIC
        user32
        VC_common
        VC_log
    )
endif()
