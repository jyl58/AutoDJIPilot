/*
* @file LinuxHelper.cpp
* @author: JYL
* @email:jiyingliang369@126.com
* @date: 2019.06.10
*
*/

#include "linux/LinuxHelpers.hpp"
#include "Message.h"

using namespace DJI::OSDK;

LinuxSetup::LinuxSetup(std::shared_ptr<LuaParser> lua_parser_pointer,const std::string& config_file_path, bool enableAdvancedSensing)
:functionTimeout(1),
vehicle(nullptr),
environment(nullptr),
useAdvancedSensing(enableAdvancedSensing)
{
	setupEnvironment(lua_parser_pointer,config_file_path);
}
std::shared_ptr<LinuxSetup> LinuxSetup::getLinuxSetupIntacne(std::shared_ptr<LuaParser> lua_parser_pointer,const std::string& config_file_path){
	return std::shared_ptr<LinuxSetup>(new LinuxSetup(lua_parser_pointer,config_file_path));
}
LinuxSetup::~LinuxSetup(){
  if (vehicle != nullptr){
    vehicle.reset();
  }
  if (environment != nullptr){
    delete (environment);
    environment = nullptr;
  }
}

void LinuxSetup::setupEnvironment(std::shared_ptr<LuaParser> lua_parser_pointer,const std::string& dji_config_file_path){
	if (!dji_config_file_path.empty()){
		std::ifstream config_file_handle(dji_config_file_path);
		if (!config_file_handle.is_open()){
			DERR(__FILE__,__LINE__,"User configuration file not open.");
		}
		config_file_handle.close();
	}

	this->environment = new LinuxEnvironment(lua_parser_pointer,dji_config_file_path);
	if (!environment->getConfigResult()){
    // We were unable to read the config file. Exit.
    	DERR(__FILE__,__LINE__,"User configuration file is not correctly formatted.");
	}
}

bool LinuxSetup::initVehicle(){
	bool threadSupport = true;
	this->vehicle      = std::shared_ptr<DJI::OSDK::Vehicle>(new Vehicle(environment->getDevice().c_str(),
		                             									 environment->getBaudrate(),
		                             									 threadSupport,
		                             									 this->useAdvancedSensing)
		                             						);

	if(this->vehicle==nullptr){
		FLIGHTLOG("Creat new vehile err.");
		return false;
	}
	// Check if the communication is working fine
	if (!vehicle->protocolLayer->getDriver()->getDeviceStatus()){
		FLIGHTLOG("Communicate with DJI incorrectly.");
		this->vehicle.reset();
		this->vehicle     = nullptr;
		return false;
	}

	  // Activate
	activateData.ID = environment->getApp_id();
	char app_key[65];
	activateData.encKey = app_key;
	strcpy(activateData.encKey, environment->getEnc_key().c_str());
	activateData.version = vehicle->getFwVersion();
	ACK::ErrorCode ack   = vehicle->activate(&activateData, functionTimeout);
	if (ACK::getError(ack)){
		FLIGHTLOG("DJI Vehicle activate fail!");
		ACK::getErrorCodeMessage(ack, __func__);
		this->vehicle.reset();
		this->vehicle     = nullptr;
		return false;
	}else{
		FLIGHTLOG("DJI Vehicle activate successful!");
	}
	return true;
}

