cmake_minimum_required(VERSION 3.0)
project(iana-tld-extractor VERSION 1.0.1 LANGUAGES C)

option(WITH_IDN2 "Build with libidn2" OFF)
option(WITH_CURL "Build with libcurl" ON)

set(CMAKE_C_STANDARD 99)
set(PROJECT_INSTALL_DLL_EXE_DIR "bin")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
add_definitions(-O2 -std=c99 -pedantic -D_GNU_SOURCE)
add_definitions(-Wall -Wextra)
add_definitions(-DAPP_VERSION=${CMAKE_PROJECT_VERSION})

set(IDNKIT_DIR "/usr/local" CACHE STRING "Path where idnkit was installed")

file(GLOB_RECURSE sources src/*.c src/*.h)
add_subdirectory(myhtml)
set_target_properties(myhtml_static PROPERTIES IMPORTED_LOCATION myhtml/lib/myhtml_static.a)
add_executable(iana-tld-extractor ${sources})
target_link_libraries(iana-tld-extractor PUBLIC myhtml_static)
target_include_directories(iana-tld-extractor PUBLIC
  "${PROJECT_BINARY_DIR}"
  "${MYHTML_INCLUDE_DIRS}"
  )

find_package(LibIDN2)
if (WITH_IDN2 AND LIBIDN2_FOUND)
  add_definitions(-DHAVE_IDN2)
  include_directories(${LIBIDN2_INCLUDE_DIR})
  target_link_libraries(iana-tld-extractor PUBLIC ${LIBIDN2_LIBRARIES})
else()
  add_library(idnkit SHARED IMPORTED)
  include_directories(${IDNKIT_DIR}/include)
  set_target_properties(idnkit PROPERTIES
    IMPORTED_LOCATION "${IDNKIT_DIR}/lib/libidnkit.so"
    INTERFACE_INCLUDE_DIRECTORIES "${IDNKIT_DIR}/include"
    )
  target_link_libraries(iana-tld-extractor PUBLIC idnkit)
endif()

find_package(CURL)
if (WITH_CURL AND CURL_FOUND)
  add_definitions(-DHAVE_CURL)
  include_directories(${CURL_INCLUDE_DIRS})
  target_link_libraries(iana-tld-extractor PUBLIC ${CURL_LIBRARIES})
endif()

install(TARGETS iana-tld-extractor DESTINATION bin)
