file(GLOB_RECURSE ALGORITHM_SOURCES *.hpp *.h)

add_library(VC_algorithm OBJECT ${ALGORITHM_SOURCES})

# 配置该模块的头文件路径：PUBLIC 属性允许调用者自动获得路径
target_include_directories(VC_algorithm PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)

# 该模块特定的依赖
target_link_libraries(VC_algorithm PUBLIC
    VC_common
)
