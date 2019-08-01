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
extern int SocketPrintFd;

#define lua_writestring(s,l) 								\
	{														\
		if(SocketPrintFd != -1){							\
			send(SocketPrintFd,s,l,0);					    \
		}else{												\
			fwrite((s),sizeof(char),(l),stdout);			\
			fflush(stdout);									\
		}													\
	}														
	
#define lua_writeline()		lua_writestring("\n",1)

#endif

