find_package(OpenSSL REQUIRED)
find_package(Event REQUIRED)

set (LINKLIBS
    "-Wl,-zrelro"
    ${OPENSSL_LIBRARIES}
    ${EVENT_LIBRARIES}
)

set (TARGET "race")
set (RACE_VERSION_MAJOR 0)
set (RACE_VERSION_MINOR 9)
set (RACE_VERSION_PATCH 0)
set (RACE_VERSION_FULL
    ${RACE_VERSION_MAJOR}.${RACE_VERSION_MINOR}.${RACE_VERSION_PATCH})
set (RACE_VERSION ${RACE_VERSION_MAJOR}.${RACE_VERSION_MINOR})

include_directories(
    include
    ${OPENSSL_INCLUDE_DIR}
    ${EVENT_INCLUDE_DIR}
    src
)
set (SOURCES
    ./src/race.c
    ./src/race_utils.c
    ./src/rraw.c
    ./src/race_module.c
)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

install(DIRECTORY
  include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/race
)

add_library(${TARGET} STATIC ${SOURCES})
target_link_libraries(${TARGET} ${LINKLIBS})
