cmake_minimum_required(VERSION 3.15)
project(rt_voxels LANGUAGES CXX C)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

add_compile_options(-Wall -Wextra -O0 -g)

# Source files
file(
    GLOB SRCS 
    src/*.cpp 
)
file(
    GLOB C_SRCS
)

find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)
find_package(GLEW REQUIRED)

add_executable(${PROJECT_NAME} ${SRCS} ${SRCS})
target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
    SDL2 
    OpenGL::GL
    GLEW::GLEW
    ${CMAKE_DL_LIBS}  # For Linux DL library
)

