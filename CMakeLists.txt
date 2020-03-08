cmake_minimum_required(VERSION 3.10)
project(iana-tld-extractor C)

option(WITH_IDN2 "Build with libidn2" OFF)

set(CMAKE_C_STANDARD 99)
set(PROJECT_INSTALL_DLL_EXE_DIR "bin")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
add_definitions(-Wall -pedantic)

set(IDNKIT_DIR "/usr/local" CACHE STRING "Path where idnkit was installed")

file(GLOB_RECURSE sources src/*.c src/*.h)
add_subdirectory(myhtml)
add_executable(iana-tld-extractor ${sources})
target_link_libraries(iana-tld-extractor PUBLIC ${MYHTML_LIBRARIES})
target_include_directories(iana-tld-extractor PUBLIC
  "${PROJECT_BINARY_DIR}"
  "${MYHTML_INCLUDE_DIRS}"
  )

find_package(LibIDN2)
if (LIBIDN2_FOUND AND WITH_IDN2)
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
if (CURL_FOUND)
  add_definitions(-DHAVE_CURL)
  include_directories(${CURL_INCLUDE_DIRS})
  target_link_libraries(iana-tld-extractor PUBLIC ${CURL_LIBRARIES})
endif()

install(TARGETS iana-tld-extractor DESTINATION bin)
