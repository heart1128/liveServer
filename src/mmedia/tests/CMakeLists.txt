add_executable(HandShakeClientTest HandShakeClientTest.cpp)
target_link_libraries(HandShakeClientTest base network mmedia crypto) # cryto是openssl的加密库

add_executable(HandShakeServerTest HandShakeServerTest.cpp)
target_link_libraries(HandShakeServerTest base network mmedia crypto)

add_executable(RtmpServerTest RtmpServerTest.cpp)
target_link_libraries(RtmpServerTest base network mmedia crypto)

# crypto是SH256加密的
add_executable(RtmpClientTest RtmpClientTest.cpp)
target_link_libraries(RtmpClientTest base network mmedia crypto)