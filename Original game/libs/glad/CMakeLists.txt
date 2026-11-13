cmake_minimum_required(VERSION 3.6)

project(glad)

set(SRC_FILES
    src/gl.c
    src/egl.c
)

if(UNIX AND NOT APPLE)
    list(APPEND SRC_FILES
        src/glx.c
    )
endif()

add_library(glad STATIC ${SRC_FILES})

target_include_directories(glad
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
