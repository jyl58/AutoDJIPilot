/*
* @file Lua_config.h
* @author: JYL
* @email:jiyingliang369@126.com
* @date: 2019.07.25
*
*/
#ifndef lua_config_h
#define lua_config_h

#include <stddef.h>
#include <stdio.h>
#include <sys/socket.h>
//lua print for socket fd
extern int luaSocketPrintFd;

#define lua_writestring(s,l) 								\
	{														\
		if(luaSocketPrintFd != -1){							\
			send(luaSocketPrintFd,s,l,0);					\
		}else{												\
			fwrite((s),sizeof(char),(l),stdout);			\
			fflush(stdout);									\
		}													\
	}														
	
#define lua_writeline()		lua_writestring("\n",1)

#endif

