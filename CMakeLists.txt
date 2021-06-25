cmake_minimum_required(VERSION 3.14)

set(PROJECT_NAME LTCImplementation)

project(${PROJECT_NAME})

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

find_package(OpenGL REQUIRED)

include_directories(/usr/local/include)

include_directories(${CMAKE_SOURCE_DIR}/include)
link_directories(${CMAKE_SOURCE_DIR}/lib)
link_directories(${CMAKE_SOURCE_DIR}/dll)

file(GLOB SOURCES ${CMAKE_SOURCE_DIR}/*.cpp *.c *.glsl)
file(GLOB HEADERS ${CMAKE_SOURCE_DIR}/*.h)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
target_link_libraries(${PROJECT_NAME}
	glfw3.dll
	glfw3.lib
	glfw3_mt.lib
	glfw3dll.lib
)