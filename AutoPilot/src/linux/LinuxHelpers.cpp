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

LinuxSetup::LinuxSetup(LuaParser* lua_parser_pointer,const std::string& config_file_path, bool enableAdvancedSensing){
	this->functionTimeout     = 1; // second
	this->vehicle             = nullptr;
	this->environment         = nullptr;
	this->useAdvancedSensing  = enableAdvancedSensing;

	setupEnvironment(lua_parser_pointer,config_file_path);
}

LinuxSetup::~LinuxSetup(){
  if (vehicle){
    delete (vehicle);
    vehicle = nullptr;
  }
  if (environment){
    delete (environment);
    environment = nullptr;
  }
}

void LinuxSetup::setupEnvironment(LuaParser* lua_parser_pointer,const std::string& dji_config_file_path){
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
	this->vehicle      = new Vehicle(environment->getDevice().c_str(),
		                             environment->getBaudrate(),
		                             threadSupport,
		                             this->useAdvancedSensing);

	if(this->vehicle==nullptr){
		FLIGHTLOG("Creat new vehile err.");
		return false;
	}
	// Check if the communication is working fine
	if (!vehicle->protocolLayer->getDriver()->getDeviceStatus()){
		FLIGHTLOG("Communicate with DJI incorrectly.");
		delete (vehicle);
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
		delete (vehicle);
		this->vehicle     = nullptr;
		return false;
	}else{
		FLIGHTLOG("DJI Vehicle activate successful!");
	}
	return true;
}

