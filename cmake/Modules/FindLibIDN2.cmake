# - Try to find LibIDN2
# Once done this will define
#  LIBIDN2_FOUND - System has LibXml2
#  LIBIDN2_INCLUDE_DIR - The LibXml2 include directories
#  LIBIDN2_LIBRARIES - The libraries needed to use LibIDN2
#  LIBIDN2_DEFINITIONS - Compiler switches required for using LibIDN2

find_package(PkgConfig)
pkg_check_modules(PC_LIBIDN2 QUIET libidn-2.0)
set(LIBIDN2_DEFINITIONS ${PC_LIBIDN2_CFLAGS_OTHER})

find_path(LIBIDN2_INCLUDE_DIR idn2.h
          HINTS ${PC_LIBIDN2_INCLUDEDIR} ${PC_LIBIDN2_INCLUDE_DIRS}
          PATH_SUFFIXES libidn2 )

find_library(LIBIDN2_LIBRARY NAMES idn2 libidn2
             HINTS ${PC_LIBIDN2_LIBDIR} ${PC_LIBIDN2_LIBRARY_DIRS} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBIDN2_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(LibIDN2  DEFAULT_MSG
                                  LIBIDN2_LIBRARY LIBIDN2_INCLUDE_DIR)

mark_as_advanced(LIBIDN2_INCLUDE_DIR LIBIDN2_LIBRARY )

set(LIBIDN2_LIBRARIES ${LIBIDN2_LIBRARY} )
set(LIBIDN2_INCLUDE_DIRS ${LIBIDN2_INCLUDE_DIR} )
