cmake_minimum_required(VERSION 2.8.12)

project(dlz_mysqlmm)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package(MySQL REQUIRED)

set(dlz_mysqlmm_SOURCES
	dlz_mysqlmm.cpp
	dlz_mysqlmm.h)

add_library(dlz_mysqlmm MODULE
	${dlz_mysqlmm_SOURCES})
target_link_libraries(dlz_mysqlmm PRIVATE
	${MYSQL_LIBRARIES})
target_include_directories(dlz_mysqlmm SYSTEM PRIVATE
	${MYSQL_INCLUDE_DIRS})