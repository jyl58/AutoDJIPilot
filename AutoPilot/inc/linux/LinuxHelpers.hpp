/*! @file dji_linux_helpers.hpp
 *  @version 3.3
 *  @date Jun 05 2019
 *
 *  @brief
 *  Helper functions to handle user configuration parsing, version query and
 * activation.
 *
 *  @Copyright (c) 2019 JYL
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#ifndef ONBOARDSDK_HELPERS_H
#define ONBOARDSDK_HELPERS_H

#include <fstream>
#include <dji_vehicle.hpp>
#include "LinuxEnvironment.hpp"
#include "LuaParser.h"

class LinuxSetup
{
public:
  LinuxSetup(LuaParser* lua_parser_pointer,const std::string& config_file_path, bool enableAdvancedSensing = false);
  ~LinuxSetup();

public:
  void setupEnvironment(LuaParser* lua_parser_pointer,const std::string& config_file_path);
  bool initVehicle();

public:
	void setTestSerialDevice(DJI::OSDK::LinuxSerialDevice* serialDevice);
	LinuxEnvironment* getEnvironment(){
		return this->environment;
	}
	DJI::OSDK::Vehicle* getVehicle(){
		return this->vehicle;
	}
	DJI::OSDK::Vehicle::ActivateData* getActivateData(){
		return &activateData;
	}
	const std::string& getMAVlinkDevPort()const{
		return environment->getMAVlinkDevPort();
	}
	unsigned int getMAVlinkDevPortBaudrate(){
		return 	environment->getMAVlinkBaudrate();
	}
	int getDJIAppId(){
		return environment->getApp_id();
	}
	const std::string& getDJIAppKey()const {
		return environment->getEnc_key();
	}
	const std::string& getDJIDevicePort()const{
		return environment->getDevice();	
	}
	unsigned int getDJIDevicePortBaudrate()const{
		return environment->getBaudrate();
	}
private:
	DJI::OSDK::Vehicle*              vehicle=nullptr;
	LinuxEnvironment*                environment;
	DJI::OSDK::Vehicle::ActivateData activateData;
	int                              functionTimeout; // seconds
	bool                             useAdvancedSensing;
};

#endif // ONBOARDSDK_HELPERS_H
