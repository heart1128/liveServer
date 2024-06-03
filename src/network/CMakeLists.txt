# 添加文件，目录下的全部
aux_source_directory(. DIR_LIB_SRCS) 
aux_source_directory(base DIR_LIB_SRCS) 
aux_source_directory(net DIR_LIB_SRCS) 
# 要编译成lib的文件，引用目录下的全部文件
add_library(network ${DIR_LIB_SRCS})

target_link_libraries(network base) # 用到了log，依赖base库

add_subdirectory(net/tests)