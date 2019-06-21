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
#include "FlightLog.h"

#define AUTO_DBUG 1
//debug
#define MESSAGEFORMATE __FILE__<<" "<<__FUNCTION__<<" "<<__LINE__<<":"
#define DERR(msg)   \
	{														\
		if (AUTO_DBUG) std::cout<<"[ERR]"<<MESSAGEFORMATE<<msg<<std::endl; \
		throw std::runtime_error(msg);						\
		FLIGHTLOG(msg);										\
	}														\

#define DWAR(msg) 											\
	{														\
		if (AUTO_DBUG) std::cout<<"[WAR]"<<MESSAGEFORMATE<<msg<<std::endl;	\
		FlightLog::writeLogBufferWithLabel(msg);										\
															\
	}
#define DDBUG(msg) 					\
	if(AUTO_DBUG)	{std::cout<<"[DBUG]"<<msg<<std::endl; }\

// log 
#define FLIGHTLOG(msg) FlightLog::writeLogBuffer(msg)
