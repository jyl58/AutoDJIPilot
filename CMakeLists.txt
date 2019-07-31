##################################
# Project top CMakeLists.txt file
# author: 	jiyingliang
# email : 	jiyingliang369@126.com
# date  :	2019.5.28
# Modify:	
##################################
cmake_minimum_required(VERSION 3.0)

OPTION(DEBUG "Debug without link dji vehicle" OFF)
if (DEBUG)
	add_definitions(-DOFFLINE_DEBUG)
	message("make for offline debug mode.")
endif()

SET(ADVANCED_SENSING 0)
if (ADVANCED_SENSING)
	find_package(LibUSB REQUIRED)
	include_directories(${LIBUSB_1_INCLUDE_DIRS})
endif()

add_subdirectory(lib/Onboard-SDK/)
add_subdirectory(lib/lua-5.3.5/)
add_subdirectory(AutoPilot/)

#set the app lua script path
file(GLOB LUA_APP_SCRIPT_FILE
	APP/bin/*.lua
)

install(FILES ${LUA_APP_SCRIPT_FILE} DESTINATION /usr/local/share/AutoDjiPilot)

file(GLOB LUA_LIB_SCRIPT_FILE
	APP/lib/*.lua	
)
install(FILES ${LUA_LIB_SCRIPT_FILE} DESTINATION /usr/local/share/AutoDjiPilot/lib)
