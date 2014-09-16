# Defines the following vars if successfull:
#  BIND9_INCLUDE_DIRS
#  BIND9_LIBRARIES
#  BIND9_FOUND

find_path(BIND9_INCLUDE_DIR
	NAMES
		isc/result.h dns/version.h bind9/version.h)

find_library(BIND9_LIB
	NAMES
		bind9 libbind9)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Bind9 DEFAULT_MSG BIND9_LIB BIND9_INCLUDE_DIR)
mark_as_advanced(BIND9_LIB BIND9_INCLUDE_DIR)

if(BIND9_FOUND)
	set(BIND9_LIBRARIES ${BIND9_LIB})
	set(BIND9_INCLUDE_DIRS ${BIND9_INCLUDE_DIR})
endif()
