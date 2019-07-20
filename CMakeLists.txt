cmake_minimum_required(VERSION 2.8.2 FATAL_ERROR)
cmake_policy(VERSION 2.8)

include(cmake/modules/GolangSimple.cmake)

set (CMAKE_C_FLAGS "-Werror -Wall -Wextra -Wno-unused-parameter -D_GNU_SOURCE -std=c11 -O3 -g ${CMAKE_C_FLAGS}")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

include(GNUInstallDirs)

add_subdirectory(lib)
add_subdirectory(racepwn)
