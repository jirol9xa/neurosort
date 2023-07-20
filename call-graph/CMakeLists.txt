cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)

option(FORCE_COLORED_OUTPUT "Always produce ANSI-colored output (GNU/Clang only)." TRUE)

# Paste it in ur own cmake files, if u re not using divine clang compiler
if (${FORCE_COLORED_OUTPUT})
  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    add_compile_options(-fdiagnostics-color=always)
  elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_compile_options(-fcolor-diagnostics)
  endif()
endif()
# Paste end

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}" -O3 -mavx2)
set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG}"   -O0 -Wall -Wextra -Wpedantic 
                            -fno-omit-frame-pointer -g -fsanitize=address,leak,undefined
                            -fsanitize-address-use-after-scope)

include_directories(include)

set(COLLECTOR lib/Collector.cpp)

add_executable(graph-collector ${COLLECTOR})
