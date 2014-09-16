# Defines the following vars if successfull:
#  MYSQL_INCLUDE_DIRS
#  MYSQL_LIBRARIES
#  MYSQL_FOUND

find_path(MYSQL_INCLUDE_DIR
	NAMES
		mysql.h
	PATHS
		/usr/include /usr/local/include
	PATH_SUFFIXES
		mysql)

find_library(MYSQL_LIB
	NAMES
		mysqlclient_r mysqlclient
	PATHS
		/usr/lib /usr/local/lib
	PATH_SUFFIXES
		mysql)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MySQL DEFAULT_MSG MYSQL_LIB MYSQL_INCLUDE_DIR)
mark_as_advanced(MYSQL_LIB MYSQL_INCLUDE_DIR)

if(MYSQL_FOUND)
	set(MYSQL_LIBRARIES ${MYSQL_LIB})
	set(MYSQL_INCLUDE_DIRS ${MYSQL_INCLUDE_DIR})
endif()
