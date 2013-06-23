cmake_minimum_required(VERSION 2.8)

project(koku-yuv4mpeg-tracker)
ADD_EXECUTABLE(koku-yuv4mpeg-tracker main.cpp motion.cpp window.cpp yuv4mpeg.cpp)
include_directories("${PROJECT_SOURCE_DIR}/extern/lib")
set(CMAKE_CXX_FLAGS "-std=c++0x -ldl -lrt -lGL -lfftw3")
add_subdirectory(extern)
target_link_libraries(koku-yuv4mpeg-tracker pthread koku-opengl-framework)
