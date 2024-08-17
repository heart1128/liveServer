cmake_minimum_required(VERSION 2.6)
project(tmms)
set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}/bin) # 设置安装前缀
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/sbin) # 设置可执行文件存放位置，放在项目同级的bin下
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib) # 设置静态库存放位置


############ 设置编译jsoncpp库
include(ExternalProject)        # 添加外部项目
# 添加和构建外部项目
ExternalProject_Add(jsoncpp     
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/third_party/jsoncpp
    CONFIGURE_COMMAND mkdir -p ../jsoncpp-build && cd ../jsoncpp-build && cmake ../jsoncpp -DCMAKE_INSTALL_PREFIX=${PROJECT_SOURCE_DIR}/lib/jsoncpp/
    BUILD_COMMAND cd ../jsoncpp-build && make
    BUILD_IN_SOURCE 1
    INSTALL_COMMAND cd ../jsoncpp-build && make install)

include_directories(${PROJECT_SOURCE_DIR}/lib/jsoncpp/include/) # 添加查找的include路径
link_directories(${PROJECT_SOURCE_DIR}/lib/jsoncpp/lib64/)  # 添加查找lib的路径
link_directories(${PROJECT_SOURCE_DIR}/lib/jsoncpp/lib/)


############ 设置编译srtp库
ExternalProject_Add(libsrtp2
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/third_party/libsrtp/
        CONFIGURE_COMMAND ./configure --prefix=${PROJECT_SOURCE_DIR}/lib/libsrtp/
        BUILD_COMMAND make
        BUILD_IN_SOURCE 1
        INSTALL_COMMAND make install
)
include_directories(${PROJECT_SOURCE_DIR}/lib/libsrtp/include/)
link_directories(${PROJECT_SOURCE_DIR}/lib/libsrtp/lib/)

############ 设置编译faad库 解码aac->pcm
ExternalProject_Add(libfaad2
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/third_party/faad2/
        #UPDATE_COMMAND ./bootstrap
        CONFIGURE_COMMAND ./bootstrap && ./configure --prefix=${PROJECT_SOURCE_DIR}/lib/faad2/
        BUILD_COMMAND make 
        BUILD_IN_SOURCE 1
        INSTALL_COMMAND make install
)
include_directories(${PROJECT_SOURCE_DIR}/lib/faad2/include/)
link_directories(${PROJECT_SOURCE_DIR}/lib/faad2/lib/)

############ 设置编译opus库 编码pcm->opus
ExternalProject_Add(libopus
        #UPDATE_COMMAND
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/third_party/opus/
        CONFIGURE_COMMAND cd ${PROJECT_SOURCE_DIR}/third_party/opus/ && pwd && autoreconf -ifv ./ && ./configure --prefix=${PROJECT_SOURCE_DIR}/lib/opus/
        #CONFIGURE_COMMAND cmake . -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${PROJECT_SOURCE_DIR}/lib/opus/
        BUILD_COMMAND make 
        BUILD_IN_SOURCE 1
        INSTALL_COMMAND make install
)
include_directories(${PROJECT_SOURCE_DIR}/lib/opus/include/)
link_directories(${PROJECT_SOURCE_DIR}/lib/opus/lib/)


if(CMAKE_BUILD_TYPE STREQUAL Debug)  #STREQUAL是字符串比较的运算符  debug 要调试信息 -g -pipi使用管道加快编译速度
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O0 -g -pipe -Wall -Werror -Wno-unused-variable -Wno-deprecated-declarations -lrt -pthread")
elseif (NOT CMAKE_BUILD_TYPE STREQUAL Debug) # 不是debug，那就是Release，-o3启动所有的编译器优化
    message(STATUS "Release Version")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O3 -g -pipe -Wall -Werror -Wno-unused-variable -Wno-deprecated-declarations -lrt -pthread")
endif()


# -fno-strict-aliasing：
# 关闭严格的别名优化。别名优化是一种编译器优化技术，它假定不同类型的指针不会指向相同的内存位置。在某些情况下，这种优化可能导致不正确的代码行为。使用这个选项可以避免这种问题，特别是在编写低级别代码或与旧代码库兼容时很有用。
# -Wno-error=shadow：
# 禁用将变量遮蔽警告视为错误。变量遮蔽（shadowing）指的是在内层作用域声明了一个与外层作用域中同名的变量，这可能导致混淆和潜在的错误。默认情况下，编译器可能会发出警告并将其视为错误，但这个选项会让编译器忽略这个警告。
# -Wno-sign-compare：
# 禁用符号比较警告。当有符号类型（例如 int）与无符号类型（例如 unsigned int）进行比较时，编译器会发出警告，因为这可能导致未定义行为或意外的结果。这个选项会让编译器忽略这种警告。
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-aliasing -Wno-error=shadow -Wno-sign-compare")

# STATUS 是消息类型，表示这是一个普通的状态消息，通常用于向用户显示信息而不是错误或警告。
message(STATUS "USING GCC VERSION: ${CMAKE_CXX_COMPILER_VERSION}")

# VERSION_GREATER 是一个 CMake 操作符，用于比较版本号。
if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 7.2.9)
    message(STATUS "GCC Version > 7.3.0, USING -Wno-error=unused-result -Werror=return-type")
    set(CMAKE_CXX_FLAGS "-Wno-error=unused-result -Werror=return-type ${CMAKE_CXX_FLAGS}")
endif()


# 添加include包含目录，在其中查找头文件
include_directories(src)
# 添加cmake子目录，会在 src 目录中查找 CMakeLists.txt，并处理其内容。
add_subdirectory(src)