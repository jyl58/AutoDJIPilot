/*
* @file Main.cpp
* @author: JYL
* @email:jiyingliang369@126.com
* @date: 2019.06.27
*
*/
/*
* @file Commander.h
* @author: JYL
* @email:jiyingliang369@126.com
* @date: 2019.06.27
*
*/
#pragma once
#include <string>
#include <iostream>
#include <stdlib.h>
#include <vector>
#include "LuaParser.h"
#include "FlightCore.h"
#include "MavlinkRouter.h"
#include "FlightLog.h"
#include "LuaInterface.h"
#include "PayloadBase.h"
#include "ConsoleServer.h"
#include "linux/LinuxHelpers.hpp"
#include "geo.h"

#define DEFAULT_LUA_DIRECTORY "/usr/local/share/AutoDjiPilot/"
typedef void (*cmdfunction)();
typedef struct CMDFUNCTION{
	char cmd_name[10];
	cmdfunction cmd_function;
}command_function_t;

class Commander{
public:
	//Do not allow construct
	Commander()=delete;
	Commander(const Commander&)=delete;
	Commander& operator =(const Commander&)=delete;
	~Commander(){}
	static void AutopilotSystemInit(const std::string& config_file_path);
	static void AutopilotSystemExit();
	static bool splitCMDAndParam(const std::string& input_stream);
	static void RunCommand(int print_fd=-1);
	static bool main_thread_need_exit;
	static bool tcp_link_need_disconnect;

private:
	static void HelpCommandCMD();
	static void ZoomCamera();
	static void	LoadPayloadPlugin();
	static void RunLuaScript();
	static void PauseRunLuaScript();
	static void GoOnRunLuaScript();
	static void BreakRunLuaScript();
	static void SetGimbal();
	static void RunVideo();
	static void ShootPhoto();
	static void PrintConfigMsgCMD();
	static void PrintFlightStatusCMD();
	static void ExitSystemCMD();
	
	static std::shared_ptr<ConsoleServer> _console_server;	
	static std::shared_ptr<LinuxSetup> _linux_setup;
	static std::shared_ptr<FlightCore> _flight_core;
	static std::shared_ptr<LuaParser> _lua_parser;
	static PayloadBase* _payload_base;
	
	static const command_function_t cmd_table[];
	static const char* cmd_description[];
	static std::vector<std::string> _cmd_and_param;

	//dynamic lib 
	static void* dynamic_lib_handler;
};
