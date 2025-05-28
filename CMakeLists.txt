cmake_minimum_required(VERSION 4.0)

project(ffmpeg-gui VERSION 1.0 LANGUAGES C)
set(CMAKE_C_STANDARD 99)

# Adding Raylib
include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE) # don't build the supplied example games

FetchContent_Declare(
    raylib
    GIT_REPOSITORY "https://github.com/raysan5/raylib.git"
    GIT_TAG "5.5"
    GIT_PROGRESS TRUE
    GIT_SHALLOW TRUE
)
FetchContent_Declare(nfd
    GIT_REPOSITORY "https://github.com/btzy/nativefiledialog-extended.git"
    GIT_TAG "v1.2.1"
    GIT_PROGRESS TRUE
    GIT_SHALLOW TRUE
)
    
FetchContent_MakeAvailable(raylib nfd)

file(GLOB SOURCES "src/${PROJECT_NAME}.c")

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE raylib
    PRIVATE nfd    
)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
)