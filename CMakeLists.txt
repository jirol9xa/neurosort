cmake_minimum_required(VERSION 3.12)

project(elf-parser)

set (INCLUDEDIR include/)
set (SRCDIR src/)

set(CMAKE_CXX_STANDARD 20)

SET(CMAKE_CXX_FLAGS "-Wall -g -ggdb3")

add_library(libparser SHARED ${SRCDIR}/parser.cpp ${SRCDIR}/lib.cpp)
target_include_directories(libparser PRIVATE ${INCLUDEDIR})

add_executable(${PROJECT_NAME} ${SRCDIR}/main.cpp ${SRCDIR}/lib.cpp ${SRCDIR}/parser.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDEDIR})

find_program(CLANG_FORMAT "clang-format")
if(CLANG_FORMAT)
    file(GLOB_RECURSE ALL_CXX_SOURCE_FILES
        ${PROJECT_SOURCE_DIR}/*.[ch]pp
        ${PROJECT_SOURCE_DIR}/*.[ch]
    )

    add_custom_target(
        clang-force-format
        COMMAND clang-format
        -i
        -style=file
        ${ALL_CXX_SOURCE_FILES}
    )
endif()
