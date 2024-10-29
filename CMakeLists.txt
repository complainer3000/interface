cmake_minimum_required(VERSION 3.16.0)

project(interface)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.h)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

add_subdirectory(Dependencies)

target_link_directories(${PROJECT_NAME} PRIVATE src)

#target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_DL_LIBS})