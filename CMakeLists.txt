cmake_minimum_required(VERSION 3.10)
project(MalaysianOsint C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)


include_directories(${CMAKE_SOURCE_DIR}/include)

set(SOURCES
    src/pdrm.c
    src/sspi.c
    src/mykad.c
    src/rmp_wanted.c
    my_osint.c
)

find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBCURL REQUIRED libcurl)
pkg_check_modules(MICROHTTPD REQUIRED libmicrohttpd)
pkg_check_modules(CJSON REQUIRED libcjson)

include_directories(
    ${LIBCURL_INCLUDE_DIRS}
    ${MICROHTTPD_INCLUDE_DIRS}
    ${CJSON_INCLUDE_DIRS}
)

link_directories(
    ${LIBCURL_LIBRARY_DIRS}
    ${MICROHTTPD_LIBRARY_DIRS}
    ${CJSON_LIBRARY_DIRS}
)


add_executable(mo ${SOURCES})

target_link_libraries(mo
    ${LIBCURL_LIBRARIES}
    ${MICROHTTPD_LIBRARIES}
    ${CJSON_LIBRARIES}
)

install(TARGETS mo DESTINATION bin)
install(DIRECTORY include/ DESTINATION include)
