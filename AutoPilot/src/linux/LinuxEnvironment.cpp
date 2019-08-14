/*
* @file LinuxEnvironment.cpp
* @author: JYL
* @email:jiyingliang369@126.com
* @date: 2019.06.10
*
*/
#include "linux/LinuxEnvironment.hpp"
#include "Message.h"
LinuxEnvironment::LinuxEnvironment(std::shared_ptr<LuaParser> lua_parser_pointer,const std::string& config_file_path)
:_luaParser(lua_parser_pointer)
{

	this->config_file_path   = config_file_path;
	this->config_read_result = this->parse(config_file_path);
}
LinuxEnvironment::~LinuxEnvironment()
{
}

int
LinuxEnvironment::getApp_id() const{
  return app_id;
}

const std::string&
LinuxEnvironment::getEnc_key() const{
  return enc_key;
}

const std::string&
LinuxEnvironment::getDevice() const{
  return device;
}

unsigned int
LinuxEnvironment::getBaudrate() const{
  return baudrate;
}

bool 
LinuxEnvironment::getConfigResult() const{
  return config_read_result;
}
const std::string& 
LinuxEnvironment::getMAVlinkDevPort()const{
	return mavlinkPort;
}
unsigned int  
LinuxEnvironment::getMAVlinkBaudrate() const{
	return mavlink_baudrate;
}
bool LinuxEnvironment::parse(const std::string& config_file_path){	
	std::string config_value;
	std::string log_context;
	FLIGHTLOG(("Parse the config file: "+config_file_path));
	// run the lua config file,do not a new thread
	_luaParser->LuaScriptOpenAndRun(config_file_path);
	//get DJI device config
	if(!_luaParser->LuaGettableValueByName("DJI_CONFIG","app_id",config_value))
		return false;
	log_context.append("DJI app id="+config_value+"\n");
	this->app_id=std::stoi(config_value);

	if(!_luaParser->LuaGettableValueByName("DJI_CONFIG","app_key",this->enc_key))
		return false;
	log_context.append("DJI app key="+this->enc_key+"\n");
	
	if(!_luaParser->LuaGettableValueByName("DJI_CONFIG","device_port",this->device))
		return false;
	log_context.append("DJI device port="+this->device+"\n");
	
	config_value.clear();
	if(!_luaParser->LuaGettableValueByName("DJI_CONFIG","baudrate",config_value))
		return false;
	log_context.append("DJI device baudrate="+config_value + "\n");
	this->baudrate=std::stoi(config_value);
	
	// get Mavlink config
	if(!_luaParser->LuaGettableValueByName("MAVLINK_CONFIG","device_port",this->mavlinkPort))
		return false;
	log_context.append("MAV device port="+this->mavlinkPort+"\n");
	config_value.clear();
	if(!_luaParser->LuaGettableValueByName("MAVLINK_CONFIG","baudrate",config_value))
		return false;
	log_context.append("MAV device port baudrate="+ config_value+ "\n");
	this->mavlink_baudrate=std::stoi(config_value);
	
	FLIGHTLOG("The Config Param:\n"+log_context);
	return true;
}

