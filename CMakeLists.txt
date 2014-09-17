cmake_minimum_required(VERSION 2.8.12)

project(dlz_mysqlmm)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules/")

set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

find_package(Threads REQUIRED)
find_package(MySQLConnCpp REQUIRED)
find_package(Bind9 REQUIRED)

set(jsoncpp_SOURCES
	jsoncpp.cpp)

set(dlz_mysqlmm_SOURCES
	dlz_mysqlmm.cpp
	dlz_mysqlmm_unused.cpp
	dlz_mysqlmm.h
	mysqlmm_manager.cpp
	mysqlmm_manager.h
	util.h)

add_library(jsoncpp
	${jsoncpp_SOURCES})
target_compile_options(jsoncpp PRIVATE
	-fvisibility=hidden -Wall -fPIC -DPIC)

add_library(dlz_mysqlmm MODULE
	${dlz_mysqlmm_SOURCES})
target_include_directories(dlz_mysqlmm SYSTEM PRIVATE
	${MYSQLCONNCPP_INCLUDE_DIRS}
	${BIND9_INCLUDE_DIRS})
target_compile_options(dlz_mysqlmm PRIVATE
	-std=gnu++11 -fvisibility=hidden
	-Wall -Wextra -pedantic)
target_link_libraries(dlz_mysqlmm PRIVATE
	jsoncpp
	${MYSQLCONNCPP_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT})
