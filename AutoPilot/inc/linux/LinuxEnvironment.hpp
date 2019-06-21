/*
* @file LinuxEnvironment.h
* @author: JYL
* @email:jiyingliang369@126.com
* @date: 2019.06.10
*
*/

#pragma once

#include <fstream>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <unistd.h>
#include "LuaParser.h"

class LinuxEnvironment{
public:
	LinuxEnvironment(LuaParser* lua_parser_pointer,const std::string& config_file_path);
	~LinuxEnvironment();
	int                getApp_id() const;
	const std::string& getEnc_key() const;
	const std::string& getDevice() const;
	unsigned int       getBaudrate() const;
	bool               getConfigResult() const;
	const std::string& getMAVlinkDevPort()const;
	unsigned int       getMAVlinkBaudrate() const;

private:
	/*parse the user config data by run lua script file*/
	bool parse(const std::string& config_file_path);

	LuaParser*   _luaParser;
	std::string  config_file_path;
	int          app_id;
	std::string  enc_key;
	std::string  device;
	std::string  mavlinkPort;
	unsigned int mavlink_baudrate;
	unsigned int baudrate;
	bool         config_read_result;
};

