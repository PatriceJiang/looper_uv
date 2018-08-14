cmake_minimum_required(VERSION 3.0)


project("test_looper" CXX)

set(CMAKE_CXX_STANDARD 11)

include_directories("thread")
include_directories("usr/include")
link_directories("usr/lib")

set(DEPS
ws2_32 
psapi
iphlpapi
userenv
uv_a 
libssl
libcrypto
websocket)

FILE(GLOB_RECURSE LOOP_SRC thread/*.cpp thread/*.h)

add_executable(test_concurr test_concurrency.cpp ${LOOP_SRC})
target_link_libraries(test_concurr ${DEPS})

add_executable(test_evs test_events.cpp ${LOOP_SRC})
target_link_libraries(test_evs ${DEPS})




