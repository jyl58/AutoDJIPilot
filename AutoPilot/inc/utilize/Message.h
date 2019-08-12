/*
* @file Message.h
* @author: JYL
* @email:jiyingliang369@126.com
* @date: 2019.06.10
*
*/
#pragma once
#include <iostream>
#include <stdio.h>
#include <sys/socket.h>
#include "FlightLog.h"
#include "lua_config.h"

#define AUTO_DBUG 1
//debug
#define MESSAGEFORMATE __FILE__<<" "<<__FUNCTION__<<" "<<__LINE__<<":"
								
#define DDBUG(msg) 											\
	if(AUTO_DBUG)	{std::cout<<"[DBUG]"<<msg<<std::endl; }	\

// log 
#define FLIGHTLOG(msg) FlightLog::writeLogBuffer(msg)
//err message
void DERR(std::string warn_file,int line ,std::string msg);
//warning message 
void DWAR(std::string warn_file,int line ,std::string msg,int fd = -1);													
//notice 
void NOTICE_MSG(int fd,std::string msg);
//out logo
void LOGO(int fd,std::string msg);
//logo
extern const std::string AutoDjiLogo;
