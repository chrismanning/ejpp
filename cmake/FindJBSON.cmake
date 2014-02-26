# Try to find jbson
# Once done, this will define
#
#  JBSON_FOUND - system has jbson
#  JBSON_INCLUDE_DIRS - the jbson include directories

# find include dir
find_path(JBSON_INCLUDE_DIR jbson/document.hpp)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set JBSON_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(JBSON DEFAULT_MSG
                                  JBSON_INCLUDE_DIR)

mark_as_advanced(JBSON_INCLUDE_DIR JBSON_LIBRARY)

set(JBSON_INCLUDE_DIRS ${JBSON_INCLUDE_DIR})
