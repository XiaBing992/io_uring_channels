CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
PROJECT(test C CXX)

#头文件
include_directories(${PROJECT_SOURCE_DIR}/include /usr/include/)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS -pthread)
#io_uring链接库
link_directories(/usr/lib)

aux_source_directory(${PROJECT_SOURCE_DIR}/src SRC)
add_executable(test ${SRC})
target_link_libraries(test uring)
