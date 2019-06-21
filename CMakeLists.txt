##################################
# Project top CMakeLists.txt file
# author: 	jiyingliang
# email : 	jiyingliang369@126.com
# date  :	2019.5.28
# Modify:	
##################################
cmake_minimum_required(VERSION 3.0)

SET(ADVANCED_SENSING 1)
add_subdirectory(lib/Onboard-SDK/)
add_subdirectory(lib/lua-5.3.5/)
add_subdirectory(AutoPilot/)


