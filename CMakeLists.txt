cmake_minimum_required(VERSION 3.0)

project(ffmpeg-gui VERSION 1.0 LANGUAGES C)
set(CMAKE_C_STANDARD 99)

include(FetchContent)

set(FETCHCONTENT_QUIET FALSE)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

set(ENABLE_CJSON_TEST OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ENABLE_CJSON_UNINSTALL OFF CACHE BOOL "" FORCE)
set(ENABLE_CUSTOM_COMPILER_FLAGS OFF CACHE BOOL "" FORCE)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE INTERNAL "")
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1 CACHE INTERNAL "")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

FetchContent_Declare(raylib
                     GIT_REPOSITORY "https://github.com/raysan5/raylib.git"
                     GIT_TAG "6.0"
                     GIT_PROGRESS TRUE
                     GIT_SHALLOW TRUE
                     )

FetchContent_Declare(nfd
                     GIT_REPOSITORY "https://github.com/btzy/nativefiledialog-extended.git"
                     GIT_TAG "v1.2.1"
                     GIT_PROGRESS TRUE
                     GIT_SHALLOW TRUE
                     )

FetchContent_Declare(cjson
                     GIT_REPOSITORY "https://github.com/DaveGamble/cJSON.git"
                     GIT_TAG "v1.7.19"
                     GIT_PROGRESS TRUE
                     GIT_SHALLOW TRUE
                     )
    
FetchContent_MakeAvailable(raylib nfd cjson)

file(GLOB SOURCES "src/${PROJECT_NAME}.c")

add_library(layout_model STATIC src/layout_model.c)
target_include_directories(layout_model
                           PUBLIC ${CMAKE_SOURCE_DIR}/src
                           PRIVATE ${cjson_SOURCE_DIR}
                           )
target_link_libraries(layout_model
                      PUBLIC cjson
                      )

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME}
                           PRIVATE ${cjson_SOURCE_DIR}
                           PRIVATE ${CMAKE_SOURCE_DIR}/src
                           PRIVATE ${CMAKE_SOURCE_DIR}/inc
                           )

target_link_libraries(${PROJECT_NAME}
                      PRIVATE raylib
                      PRIVATE nfd
                      PRIVATE cjson
                      PRIVATE layout_model
                      )

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/resources
                       $<TARGET_FILE_DIR:${PROJECT_NAME}>/resources
                   )

enable_testing()

add_executable(layout_model_tests tests/test_layout_model.c)
target_include_directories(layout_model_tests
                           PRIVATE ${CMAKE_SOURCE_DIR}/src
                           )
target_link_libraries(layout_model_tests
                      PRIVATE layout_model
                      )
add_test(NAME layout_model_tests COMMAND layout_model_tests)
