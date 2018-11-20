cmake_minimum_required(VERSION 2.8.8 FATAL_ERROR)
cmake_policy(VERSION 2.8)

set (CMAKE_C_FLAGS "-Werror -Wall -Wextra -Wno-unused-parameter -std=gnu99 -O0 -g ${CMAKE_C_FLAGS}")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

include(GNUInstallDirs)

add_subdirectory(lib)
