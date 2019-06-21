##################################
# LUB_LIB CMakeLists.txt file
# author: 	jiyingliang
# email : 	jiyingliang369@126.com
# date  :	2019.5.22
# Modify:	
##################################

cmake_minimum_required(VERSION 3.0)

project(lua-5.3.5)

include_directories(
src/
)

SET(LUA_LIB_SRCS
src/lapi.c
src/lauxlib.c
src/lbaselib.c
src/lbitlib.c
src/lcode.c
src/lcorolib.c
src/lctype.c
src/ldblib.c
src/ldebug.c
src/ldo.c
src/ldump.c
src/lfunc.c
src/lgc.c
src/linit.c
src/liolib.c
src/llex.c
src/lmathlib.c
src/lmem.c
src/loadlib.c
src/lobject.c
src/lopcodes.c
src/loslib.c
src/lparser.c
src/lstate.c
src/lstring.c
src/lstrlib.c
src/ltable.c
src/ltablib.c
src/ltm.c
src/lundump.c
src/lutf8lib.c
src/lvm.c
src/lzio.c
)

add_library(${PROJECT_NAME} STATIC ${LUA_LIB_SRCS})
