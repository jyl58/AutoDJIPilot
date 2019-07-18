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
	this->testSerialDevice    = nullptr;
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
  if (testSerialDevice){
    delete (testSerialDevice);
    testSerialDevice = nullptr;
  }
}

void LinuxSetup::setupEnvironment(LuaParser* lua_parser_pointer,const std::string& dji_config_file_path){
	if (!dji_config_file_path.empty()){
		std::ifstream config_file_handle(dji_config_file_path);
		if (!config_file_handle.is_open()){
			DERR("User configuration file not open.");
		}
		config_file_handle.close();
	}

	this->environment = new LinuxEnvironment(lua_parser_pointer,dji_config_file_path);
	if (!environment->getConfigResult()){
    // We were unable to read the config file. Exit.
    	DERR("User configuration file is not correctly formatted.");
	}
}

bool LinuxSetup::initVehicle(){
	bool threadSupport = true;
	this->vehicle      = new Vehicle(environment->getDevice().c_str(),
		                             environment->getBaudrate(),
		                             threadSupport,
		                             this->useAdvancedSensing);

	// Check if the communication is working fine
	if (!vehicle->protocolLayer->getDriver()->getDeviceStatus()){
		std::cout << "Comms appear to be incorrectly set up. Exiting.\n";
		FLIGHTLOG("Communicate with DJI incorrectly.");
		delete (vehicle);
		delete (environment);
		this->vehicle     = nullptr;
		this->environment = nullptr;
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
		delete (environment);
		this->environment = nullptr;
		this->vehicle     = nullptr;
		return false;
	}else{
		FLIGHTLOG("DJI Vehicle activate successful!");
	}
	return true;
}

bool LinuxSetup::validateSerialPort(){
  static const int BUFFER_SIZE = 2048;

  //! Check the serial channel for data
  uint8_t buf[BUFFER_SIZE];
  if (!testSerialDevice->setSerialPureTimedRead())
  {
    DERROR("Failed to set up port for timed read.\n");
    return (false);
  };
  usleep(100000);
  if (testSerialDevice->serialRead(buf, BUFFER_SIZE)){
    DERROR("Succeeded to read from serial device\n");
  }else{
    DERROR("\"Failed to read from serial device. The Onboard SDK is not "
             "communicating with your drone. \n");
    // serialDevice->unsetSerialPureTimedRead();
    return (false);
  }

  // If we reach here, _serialRead succeeded.
  int baudCheckStatus = testSerialDevice->checkBaudRate(buf);
  if (baudCheckStatus == -1){
    DERROR("No data on the line. Is your drone powered on?\n");
    return false;
  }
  if (baudCheckStatus == -2){
    DERROR("Baud rate mismatch found. Make sure DJI Assistant 2 has the same "
             "baud setting as the one in User_Config.h\n");
    return (false);
  }
  // All the tests passed and the serial device is properly set up
  testSerialDevice->unsetSerialPureTimedRead();
  return (true);
}
