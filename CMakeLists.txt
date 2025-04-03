cmake_minimum_required(VERSION 3.5...4.0)
project(ffmpeg-gui VERSION 1.0 LANGUAGES C)

find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(SDL2_image REQUIRED)

file(GLOB SOURCES "src/*.c")
file(COPY resources DESTINATION ${CMAKE_BINARY_DIR})

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE "inc")

target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PRIVATE SDL2_ttf::SDL2_ttf)
target_include_directories(${PROJECT_NAME} PRIVATE SDL2_image::SDL2_image)

target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_LIBRARIES})
target_link_libraries(${PROJECT_NAME} PRIVATE SDL2_ttf::SDL2_ttf)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL2_image::SDL2_image)