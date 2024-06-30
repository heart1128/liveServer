aux_source_directory(. DIR_LIB_SRCS)
aux_source_directory(base DIR_LIB_SRCS)
add_library(live ${DIR_LIB_SRCS})
target_link_libraries(live base network mmedia)

# add_subdirectory(tests)