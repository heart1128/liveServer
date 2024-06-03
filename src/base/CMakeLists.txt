# 添加文件，目录下的全部
aux_source_directory(. DIR_LIB_SRCS) 
# 要编译成lib的文件，引用目录下的全部文件
add_library(base ${DIR_LIB_SRCS})

add_executable(base_test ${DIR_LIB_SRCS})
target_link_libraries(base_test base jsoncpp.a) # 链接库base