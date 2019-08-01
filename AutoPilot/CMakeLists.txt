##
# AutoPilotDJI CMakeLists.txt file
# author: 	jiyingliang
# email : 	jiyingliang369@126.com
# date  :	2019.5.28
# Modify:	
##
cmake_minimum_required(VERSION 3.0)

project(AutoPilotDJI)

add_compile_options(-std=c++11 )

find_path(ev_INCLUDE_DIR NAMES ev.h )

if (ev_INCLUDE_DIR AND EXISTS "${ev_INCLUDE_DIR}/ev.h")
	message("-- Find the ev.h at ${ev_INCLUDE_DIR}/ev.h")
endif()

if (NOT ev_LIBRARIES_DIR)
	find_library(ev_LIBRARIES_DIR NAMES libev.so)
endif ()

if(ev_LIBRARIES_DIR )
	message("-- Find the libev.a at ${ev_LIBRARIES_DIR}")
endif ()

include_directories(
inc/
inc/linux/
inc/utilize/
inc/dev/
../lib/
../lib/c_library_v2/
../lib/c_library_v2/ardupilotmega/
../lib/c_library_v2/common/
../lib/lua-5.3.5/src/
../lib/Onboard-SDK/osdk-core/api/inc/
../lib/Onboard-SDK/osdk-core/protocol/inc/
../lib/Onboard-SDK/osdk-core/hal/inc/
../lib/Onboard-SDK/osdk-core/platform/default/inc
../lib/Onboard-SDK/osdk-core/platform/linux/inc/
../lib/Onboard-SDK/osdk-core/utility/inc/
../lib/Onboard-SDK/osdk-core/advanced-sensing-2.2.0/inc/
${ev_INCLUDE_DIR}
)

set(SRC_LISTS 
src/linux/LinuxHelpers.cpp
src/linux/LinuxEnvironment.cpp
src/dev/InterfaceSerial.cpp
src/utilize/geo.cpp
src/utilize/Message.cpp
src/LuaParser.cpp
src/FlightCore.cpp
src/LuaInterface.cpp
src/FlightLog.cpp
src/MavlinkRouter.cpp
src/PayloadBase.cpp
src/ConsoleServer.cpp
src/utilize/EventManage.cpp
src/Commander.cpp
src/Main.cpp
)

add_executable(${PROJECT_NAME} 
${SRC_LISTS}
)
target_link_libraries(${PROJECT_NAME} 
	lua-5.3.5 
	djiosdk-core 
	${ev_LIBRARIES_DIR} 
	-ldl 
)
install(TARGETS ${PROJECT_NAME} DESTINATION /usr/local/bin)



