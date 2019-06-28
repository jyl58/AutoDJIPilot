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
#include "Message.h"
#include "PayloadBase.h"
#include "ConsoleServer.h"
#include "linux/LinuxHelpers.hpp"
#include "geo.h"

typedef void (*cmdfunction)(std::ostringstream&);
typedef struct CMDFUNCTION{
	char cmd_name[10];
	cmdfunction cmd_function;
}command_function_t;

class Commander{
public:
	Commander(){}
	~Commander(){}
	static void AutopilotSystemInit(const std::string& config_file_path);
	static void AutopilotSystemExit();
	static bool splitCMDAndParam(const std::string& input_stream);
	static void RunCommand(std::ostringstream& outMsg);
	static bool main_thread_need_exit;
	static bool tcp_link_need_disconnect;

private:
	static void HelpCommandCMD(std::ostringstream& outMsg);
	static void ZoomCamera(std::ostringstream& outMsg);
	static void	LoadPayloadPlugin(std::ostringstream& outMsg);
	static void RunLuaScript(std::ostringstream& outMsg);
	static void BreakRunLuaScript(std::ostringstream& outMsg);
	static void SetGimbal(std::ostringstream& outMsg);
	static void RunVideo(std::ostringstream& outMsg);
	static void ShootPhoto(std::ostringstream& outMsg);
	static void PrintConfigMsgCMD(std::ostringstream& outMsg);
	static void PrintFlightStatusCMD(std::ostringstream& outMsg);
	static void ExitSystemCMD(std::ostringstream& outMsg);
	
	static ConsoleServer* _console_server;	
	static LinuxSetup* _linux_setup;
	static FlightCore* _flight_core;
	static LuaParser* _lua_parser;
	static PayloadBase* _payload_base;
	
	
	static const command_function_t cmd_table[];
	static const char* cmd_description[];
	static std::vector<std::string> _cmd_and_param;

	//dynamic lib 
	static void* dynamic_lib_handler;
};
