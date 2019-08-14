/*
* @file FlightCore.cpp
* @author: JYL
* @email:jiyingliang369@126.com
* @date: 2019.06.10
*
*/
#include <iostream>
#include <sstream>
#include <cmath>
#include <sys/time.h>
#include "FlightCore.h"
#include "MavlinkRouter.h"
#include "LuaParser.h"
#include "geo.h"
#include "dji_control.hpp"
#include "Message.h"
//init static var
std::shared_ptr<DJI::OSDK::Vehicle> FlightCore::_vehicle=nullptr;
std::mutex FlightCore::_vehicle_data_mutex;
bool FlightCore::_vehicle_rtk_avilable=false;
TypeMap<TOPIC_STATUS_FLIGHT>::type 		FlightCore::_flightStatus;
TypeMap<TOPIC_STATUS_DISPLAYMODE>::type	FlightCore::_display_mode;
TypeMap<TOPIC_GPS_FUSED>::type 			FlightCore::_current_lat_lon;
TypeMap<TOPIC_GPS_DETAILS>::type 		FlightCore::_gps_details;
TypeMap<TOPIC_GPS_SIGNAL_LEVEL>::type 	FlightCore::_gps_signal_level;
TypeMap<TOPIC_HEIGHT_FUSION>::type 	FlightCore::_height_fusioned;
TypeMap<TOPIC_RC_FULL_RAW_DATA>::type 	FlightCore::_rc_full_raw_data;
TypeMap<TOPIC_RC>::type 				FlightCore::_rc_data;
TypeMap<TOPIC_RC_WITH_FLAG_DATA>::type 	FlightCore::_rc_witch_flag;
TypeMap<TOPIC_VELOCITY>::type 			FlightCore::_velocity;
TypeMap<TOPIC_QUATERNION>::type 		FlightCore::_quaternion;
TypeMap<TOPIC_BATTERY_INFO>::type 		FlightCore::_battery_info;
TypeMap<TOPIC_RTK_POSITION>::type 		FlightCore::_rtk_pos;
TypeMap<TOPIC_RTK_POSITION_INFO>::type 	FlightCore::_rtk_pos_info;
TypeMap<TOPIC_RTK_VELOCITY>::type 		FlightCore::_rtk_velocity;
TypeMap<TOPIC_RTK_YAW>::type			FlightCore::_rtk_yaw;
TypeMap<TOPIC_RTK_YAW_INFO>::type		FlightCore::_rtk_yaw_info;
TypeMap<TOPIC_GIMBAL_ANGLES>::type		FlightCore::_gimbal_angle;
TypeMap<TOPIC_GIMBAL_STATUS>::type		FlightCore::_gimbal_status;
TypeMap<TOPIC_GIMBAL_CONTROL_MODE>::type	FlightCore::_gimbal_mode;
const struct PERIOD_CALL_LOOP FlightCore::_period_call_tabe[]={
	{FlightCore::sendVehicleLocation,10},
	{FlightCore::sendBatteryInfo,3},
	{FlightCore::logLocation,10}
};
FlightCore::FlightCore()
:_thread_need_exit(false)
{
}

FlightCore::~FlightCore(){
	exitDjiThread();
}
/**exit the thread**/
void FlightCore::exitDjiThread(){
	if(_dji_FC_link_thread != nullptr){	
		_thread_need_exit=true;
		_dji_FC_link_thread->join();
		delete _dji_FC_link_thread;
		_dji_FC_link_thread=nullptr;
	}
	// release ctr authority
	djiReleaseControlAuthority();
}
bool
FlightCore::djiGetControlAuthority(){
	int functionTimeout=1;
	char func[50];
	if(_vehicle == nullptr){
		DWAR(__FILE__,__LINE__,"Vehicle pointer is nullptr.",SocketPrintFd);	
		return false;
	}
	//get control authority //try 3 times for roubust
	int try_times=0;
	for(try_times=0;try_times<3;try_times++){
		ACK::ErrorCode Status=_vehicle->obtainCtrlAuthority(functionTimeout);
		if (ACK::getError(Status) == ACK::SUCCESS){
			break;
		}else{
			//sleep 1s and try again
			ACK::getErrorCodeMessage(Status,func);
			sleep(1);
		}
	}
	if(try_times>=3){
		std::string errmsg(func);
		DWAR(__FILE__,__LINE__,"Get control authority err: "+errmsg,SocketPrintFd);	
		return false;
	}
	return true;
}
bool
FlightCore::djiReleaseControlAuthority(){	
	int functionTimeout=1;
	char func[50];
	if(_vehicle == nullptr){
		DWAR(__FILE__,__LINE__,"Vehicle pointer is nullptr.",SocketPrintFd);	
		return false;
	}
	//Release control authority//try 3 times for roubust
	int try_times=0;
	for(try_times=0; try_times<3; try_times++){
		ACK::ErrorCode Status=_vehicle->releaseCtrlAuthority(functionTimeout);
		if (ACK::getError(Status) == ACK::SUCCESS){
			break;
		}else{
			//sleep 1s and try again
			ACK::getErrorCodeMessage(Status,func);
			sleep(1);
		}
	}
	if(try_times>=3){
		std::string errmsg(func);
		DWAR(__FILE__,__LINE__,"Release control authority err: "+errmsg,SocketPrintFd);	
		return false;
	}
	return true;
}
bool FlightCore::flightCoreInit(std::shared_ptr<DJI::OSDK::Vehicle> vehicle){
	// init static _vehicle	
	_vehicle=vehicle;
	
	if(!djiGetControlAuthority()){
		return false;
	}
	char func[50];
	//subcribeData
	/* Flight status at 5 hz
	 * Fused lat/lon at 10 hz
	 * Fused altitude at 10hz 
	 * RC channels 	  at 50 hz
	 * velocity       at 50 hz
	 * quaternion     at 200 hz
	 * <note>:  The sub TOPIC's freq must <= the MAX value in SDK doc document  
	*/
	int responseTimeout=1;
	ACK::ErrorCode subcribeStatus;
	subcribeStatus = _vehicle->subscribe->verify(responseTimeout);
	if (ACK::getError(subcribeStatus) != ACK::SUCCESS){
		ACK::getErrorCodeMessage(subcribeStatus,func);
		std::string errmsg(func);
		DWAR(__FILE__,__LINE__,"Subscribe Verify err: "+errmsg);
		return false;
	}
	// package 0: Subscribe to flight status, display mode ,battery info at 10 hz
	int pkgIndex			=0;
	int freq 				=5;
	TopicName topicList5Hz[]	={TOPIC_STATUS_FLIGHT,TOPIC_STATUS_DISPLAYMODE,TOPIC_BATTERY_INFO,TOPIC_GPS_DETAILS};
	int  numTopic			=sizeof(topicList5Hz) / sizeof(topicList5Hz[0]);
	bool enableTimestamp	=false;
	bool pkgStatus			=_vehicle->subscribe->initPackageFromTopicList(pkgIndex,numTopic,topicList5Hz,enableTimestamp,freq);
	if(!pkgStatus){
		DWAR(__FILE__,__LINE__,"Init pkgindex 0 err.");
		return pkgStatus;
	}
	subcribeStatus			=_vehicle->subscribe->startPackage(pkgIndex,responseTimeout);
	if (ACK::getError(subcribeStatus) != ACK::SUCCESS){
		ACK::getErrorCodeMessage(subcribeStatus,func);
		_vehicle->subscribe->removePackage(pkgIndex,responseTimeout);
		std::string errmsg(func);
		DWAR(__FILE__,__LINE__,"Subscribe Start Package 0 err: "+errmsg);	
		return false;
	}
	/*register a callback for pkg index 0*/
	_vehicle->subscribe->registerUserPackageUnpackCallback(0,FlightCore::PKGIndex_0_Callback);

	//package 1: lat/lon/alt and Velocity at 10 hz
	pkgIndex				=1;
	freq 					=10;
	TopicName topicList10Hz[]	={ TOPIC_GPS_FUSED, TOPIC_HEIGHT_FUSION,TOPIC_GIMBAL_ANGLES,TOPIC_GIMBAL_STATUS,TOPIC_GIMBAL_CONTROL_MODE};
	numTopic				=sizeof(topicList10Hz)/sizeof(topicList10Hz[0]);
	enableTimestamp			=false;
	pkgStatus				=_vehicle->subscribe->initPackageFromTopicList(pkgIndex,numTopic,topicList10Hz,enableTimestamp,freq);
	if(!pkgStatus){
		DWAR(__FILE__,__LINE__,"Init pkgindex 1 err.");
		return pkgStatus;
	}
	subcribeStatus			=_vehicle->subscribe->startPackage(pkgIndex,responseTimeout);
	if (ACK::getError(subcribeStatus) != ACK::SUCCESS){
		ACK::getErrorCodeMessage(subcribeStatus, func);
		_vehicle->subscribe->removePackage(pkgIndex,responseTimeout);
		std::string errmsg(func);
		DWAR(__FILE__,__LINE__,"Subscribe Start Package 1 err: "+errmsg);	
		return false;
	}
	/*register a callback for pkg index 1*/
	_vehicle->subscribe->registerUserPackageUnpackCallback(1,FlightCore::PKGIndex_1_Callback);

	//package 2: RC channel and fusion Velocity at 20 hz
	pkgIndex				=2;
	freq 					=20;
	TopicName topicList20Hz[]	={ TOPIC_RC_FULL_RAW_DATA, TOPIC_VELOCITY,TOPIC_GPS_SIGNAL_LEVEL,TOPIC_RC,TOPIC_RC_WITH_FLAG_DATA};
	numTopic				=sizeof(topicList20Hz)/sizeof(topicList20Hz[0]);
	enableTimestamp			=false;
	pkgStatus				=_vehicle->subscribe->initPackageFromTopicList(pkgIndex,numTopic,topicList20Hz,enableTimestamp,freq);
	if(!pkgStatus){
		DWAR(__FILE__,__LINE__,"Init pkgindex 2 err.");
		return pkgStatus;
	}
	subcribeStatus			=_vehicle->subscribe->startPackage(pkgIndex,responseTimeout);
	if (ACK::getError(subcribeStatus) != ACK::SUCCESS){
		ACK::getErrorCodeMessage(subcribeStatus, func);
		_vehicle->subscribe->removePackage(pkgIndex,responseTimeout);
		std::string errmsg(func);
		DWAR(__FILE__,__LINE__,"Subscribe Start Package 2 err: "+errmsg);	
		return false;
	}
	/*register a callback for pkg index 2*/
	_vehicle->subscribe->registerUserPackageUnpackCallback(2,FlightCore::PKGIndex_2_Callback);

	//package 3: Quaternion  at 100 hz
	pkgIndex					=3;
	freq 						=100;
	TopicName topicList200Hz[]	={ TOPIC_QUATERNION };
	numTopic					=sizeof(topicList200Hz)/sizeof(topicList200Hz[0]);
	enableTimestamp				=false;
	pkgStatus					=_vehicle->subscribe->initPackageFromTopicList(pkgIndex,numTopic,topicList200Hz,enableTimestamp,freq);
	if(!pkgStatus){
		DWAR(__FILE__,__LINE__,"Init pkgindex 3 err.");
		return pkgStatus;
	}
	subcribeStatus				=_vehicle->subscribe->startPackage(pkgIndex,responseTimeout);
	if (ACK::getError(subcribeStatus) != ACK::SUCCESS){
		ACK::getErrorCodeMessage(subcribeStatus, func);
		_vehicle->subscribe->removePackage(pkgIndex,responseTimeout);
		std::string errmsg(func);
		DWAR(__FILE__,__LINE__,"Subscribe Start Package 3 err: "+errmsg);	
		return false;
	}
	/*register a callback for pkg index 3*/
	_vehicle->subscribe->registerUserPackageUnpackCallback(3,FlightCore::PKGIndex_3_Callback);
	
	//package 4: RTK  at 5 hz
	pkgIndex					=4;
	freq 						=5;
	TopicName topicListRTK5Hz[]	={ TOPIC_RTK_POSITION, TOPIC_RTK_YAW_INFO, TOPIC_RTK_POSITION_INFO,TOPIC_RTK_VELOCITY,TOPIC_RTK_YAW};
	numTopic					=sizeof(topicListRTK5Hz)/sizeof(topicListRTK5Hz[0]);
	enableTimestamp				=false;
	pkgStatus					=_vehicle->subscribe->initPackageFromTopicList(pkgIndex,numTopic,topicListRTK5Hz,enableTimestamp,freq);
	if(!pkgStatus){
		DWAR(__FILE__,__LINE__,"Init pkgindex 4 err.");
		return pkgStatus;
	}
	subcribeStatus				=_vehicle->subscribe->startPackage(pkgIndex,responseTimeout);
	if(subcribeStatus.data == ErrorCode::SubscribeACK::SOURCE_DEVICE_OFFLINE){
		FLIGHTLOG("RTK Not Available.");
		_vehicle_rtk_avilable=false;
	}else {
		_vehicle_rtk_avilable=true;
		if(ACK::getError(subcribeStatus) != ACK::SUCCESS){
			ACK::getErrorCodeMessage(subcribeStatus,func);
			_vehicle->subscribe->removePackage(pkgIndex,responseTimeout);
			std::string errmsg(func);
			DWAR(__FILE__,__LINE__,"Subscribe Start Package 4 err: "+errmsg);	
			return false;
		}
	}
	/*register a callback for pkg index 4*/
	_vehicle->subscribe->registerUserPackageUnpackCallback(4,FlightCore::PKGIndex_4_Callback);
	
	//checke if thread is running
	exitDjiThread();
	//creat a new thread for use the fc data
	_dji_FC_link_thread=new std::thread(&FlightCore::readVehicleStatusThread,this);

	return true;
}
void FlightCore::PKGIndex_0_Callback(Vehicle* vehicle,RecvContainer recvFrame,UserData usrData){
	_vehicle_data_mutex.lock();
	_flightStatus 		= vehicle->subscribe->getValue<TOPIC_STATUS_FLIGHT>();
	_display_mode		= vehicle->subscribe->getValue<TOPIC_STATUS_DISPLAYMODE>(); 
	_battery_info		= vehicle->subscribe->getValue<TOPIC_BATTERY_INFO>();
	_gps_details		= vehicle->subscribe->getValue<TOPIC_GPS_DETAILS>();
	//unlock the mutex		
	_vehicle_data_mutex.unlock();
}
void FlightCore::PKGIndex_1_Callback(Vehicle* vehicle,RecvContainer recvFrame,UserData usrData){
	_vehicle_data_mutex.lock();
	_current_lat_lon	= vehicle->subscribe->getValue<TOPIC_GPS_FUSED>();
	_height_fusioned	= vehicle->subscribe->getValue<TOPIC_HEIGHT_FUSION>();
	_gimbal_status		= vehicle->subscribe->getValue<TOPIC_GIMBAL_STATUS>();
	
	if(_gimbal_status.mountStatus){
		_gimbal_angle	= vehicle->subscribe->getValue<TOPIC_GIMBAL_ANGLES>();
		_gimbal_mode	= vehicle->subscribe->getValue<TOPIC_GIMBAL_CONTROL_MODE>();
	}
	//unlock the mutex		
	_vehicle_data_mutex.unlock();
}
void FlightCore::PKGIndex_2_Callback(Vehicle* vehicle,RecvContainer recvFrame,UserData usrData){
	_vehicle_data_mutex.lock();
	_velocity			= vehicle->subscribe->getValue<TOPIC_VELOCITY>();
	_gps_signal_level	= vehicle->subscribe->getValue<TOPIC_GPS_SIGNAL_LEVEL>();
	_rc_full_raw_data	= vehicle->subscribe->getValue<TOPIC_RC_FULL_RAW_DATA>();
	_rc_data			= vehicle->subscribe->getValue<TOPIC_RC>();
	_rc_witch_flag	    = vehicle->subscribe->getValue<TOPIC_RC_WITH_FLAG_DATA>();
	//unlock the mutex		
	_vehicle_data_mutex.unlock();
}
void FlightCore::PKGIndex_3_Callback(Vehicle* vehicle,RecvContainer recvFrame,UserData usrData){
	_vehicle_data_mutex.lock();
	_quaternion	  		=  vehicle->subscribe->getValue<TOPIC_QUATERNION>();
	//unlock the mutex		
	_vehicle_data_mutex.unlock();
}

void FlightCore::PKGIndex_4_Callback(Vehicle* vehicle,RecvContainer recvFrame,UserData usrData){
	if(_vehicle_rtk_avilable){
		_vehicle_data_mutex.lock();
		_rtk_pos	  		= vehicle->subscribe->getValue<TOPIC_RTK_POSITION>();
		_rtk_pos_info	  	= vehicle->subscribe->getValue<TOPIC_RTK_POSITION_INFO>();
		_rtk_velocity	  	= vehicle->subscribe->getValue<TOPIC_RTK_VELOCITY>();
		_rtk_yaw	  		= vehicle->subscribe->getValue<TOPIC_RTK_YAW>();
		_rtk_yaw_info	  	= vehicle->subscribe->getValue<TOPIC_RTK_YAW_INFO>();
		_vehicle_data_mutex.unlock();
	}
}
void FlightCore::readVehicleStatusThread(){
	FLIGHTLOG("Start run flight core thread");
	//loop send flight status to mavlink
	//creat for run time record
	_last_run_time=new double[sizeof(_period_call_tabe)/sizeof(_period_call_tabe[0])];
	memset(_last_run_time,0,sizeof(_period_call_tabe)/sizeof(_period_call_tabe[0]));
	while(!_thread_need_exit){
		//lock the mutex 
		_vehicle_data_mutex.lock();
		//check interrupt
		checkRCInterruptLuaRun();
		//run the period function scheduler
		periodFunctionScheduler();
			
		//unlock the mutex		
		_vehicle_data_mutex.unlock();
		usleep(20000); //sleep 20 ms ~50hz
	}
	FLIGHTLOG("Exit flight core thread");
	//remove the subscribe the package	
	for (int i=0;i<MAX_PKG_COUNT;i++){
		_vehicle->subscribe->removePackage(i,1);
	}
	//delete the newed heap
	delete[] _last_run_time;
}
void 
FlightCore::periodFunctionScheduler(){
	struct timeval t;
	gettimeofday(&t,NULL);
	//ms
	double _now=t.tv_sec*1000.0+t.tv_usec*0.001;
	for(int i=0; i < sizeof(_period_call_tabe)/sizeof(_period_call_tabe[0]); i++){
		if(_now - _last_run_time[i] >= 1000.0/_period_call_tabe[i]._call_freq){
			//call period function
			_period_call_tabe[i]._func();
			_last_run_time[i]=_now;
		}
	}
}
void 
FlightCore::sendVehicleLocation(){
	//send vehicle location msg by mavlink protocol
	MavlinkRouter::sendLocation(_current_lat_lon.latitude*RAD2DEG,_current_lat_lon.longitude*RAD2DEG,_height_fusioned,_velocity.data.x,_velocity.data.y,_velocity.data.z);
}
void 
FlightCore::sendBatteryInfo(){

}
void 
FlightCore::logLocation(){
	//write vehicle location to log file
	FLIGHTLOG("Location: "+ std::to_string(1E7*_current_lat_lon.latitude*RAD2DEG)+","+
						    std::to_string(1E7*_current_lat_lon.longitude*RAD2DEG)+","+
						    std::to_string(1E3*_height_fusioned));
}
void 
FlightCore::checkRCInterruptLuaRun(){
	static int _rc_break_auto_control_count;
	if(!LuaParser::LuaScriptThreadRunning()){
		_rc_break_auto_control_count=0;
		return;
	}
	if(_rc_witch_flag.roll != 0.0f || _rc_witch_flag.pitch != 0.0f || _rc_witch_flag.yaw != 0.0f || _rc_witch_flag.throttle != 0.0f){
		_rc_break_auto_control_count++;
	}else{
		_rc_break_auto_control_count=0;
	}
	// 20 hz: 50ms*20=1000ms
	if(_rc_break_auto_control_count >= 20){
		_rc_break_auto_control_count=0;
		LuaParser::LuaInterruptRuning("RC operation.");
	}
}
void 
FlightCore::getFlightStatus(TypeMap<TOPIC_STATUS_FLIGHT>::type* flightStatus){
	_vehicle_data_mutex.lock();
	memcpy(flightStatus,&_flightStatus,sizeof(TypeMap<TOPIC_STATUS_FLIGHT>::type));
	_vehicle_data_mutex.unlock();
}
void 
FlightCore::getFlightBatteryInfo(TypeMap<TOPIC_BATTERY_INFO>::type* battery_info){
	_vehicle_data_mutex.lock();
	memcpy(battery_info,&_battery_info,sizeof(TypeMap<TOPIC_BATTERY_INFO>::type));
	_vehicle_data_mutex.unlock();
}
void 
FlightCore::getVehicleGPS(TypeMap<TOPIC_GPS_FUSED>::type* lat_lon){
	_vehicle_data_mutex.lock();
	memcpy(lat_lon,&_current_lat_lon,sizeof(TypeMap<TOPIC_GPS_FUSED>::type));
	_vehicle_data_mutex.unlock();
}
void 
FlightCore::getVehicleGpsSignalLevel(TypeMap<TOPIC_GPS_SIGNAL_LEVEL>::type* gps_signal){
	_vehicle_data_mutex.lock();
	memcpy(gps_signal,&_gps_signal_level,sizeof(TypeMap<TOPIC_GPS_SIGNAL_LEVEL>::type));
	_vehicle_data_mutex.unlock();
}
void
FlightCore::getVehicleAltitude(TypeMap<TOPIC_HEIGHT_FUSION>::type* height){
	_vehicle_data_mutex.lock();
	memcpy(height,&_height_fusioned,sizeof(TypeMap<TOPIC_HEIGHT_FUSION>::type));
	_vehicle_data_mutex.unlock();
}
void 
FlightCore::getVehicleGpsDetails(TypeMap<TOPIC_GPS_DETAILS>::type* gps_details){
	_vehicle_data_mutex.lock();
	memcpy(gps_details,&_gps_details,sizeof(TypeMap<TOPIC_GPS_DETAILS>::type));
	_vehicle_data_mutex.unlock();
}
void 
FlightCore::getVehicleVelocity(TypeMap<TOPIC_VELOCITY>::type* velocity){
	_vehicle_data_mutex.lock();
	memcpy(velocity,&_velocity,sizeof(TypeMap<TOPIC_VELOCITY>::type));
	_vehicle_data_mutex.unlock();
}
void 
FlightCore::getVehicleQuaternion(TypeMap<TOPIC_QUATERNION>::type* quaternion){
	_vehicle_data_mutex.lock();
	memcpy(quaternion,&_quaternion,sizeof(TypeMap<TOPIC_QUATERNION>::type));
	_vehicle_data_mutex.unlock();
}
void 
FlightCore::getVehicleDisplay(TypeMap<TOPIC_STATUS_DISPLAYMODE>::type*	display_mode){
	_vehicle_data_mutex.lock();
	memcpy(display_mode,&_display_mode,sizeof(TypeMap<TOPIC_STATUS_DISPLAYMODE>::type));
	_vehicle_data_mutex.unlock();
}
void	
FlightCore::getGimbalAngle(TypeMap<TOPIC_GIMBAL_ANGLES>::type	*gimbal_angle){
	_vehicle_data_mutex.lock();
	memcpy(gimbal_angle,&_gimbal_angle,sizeof(TypeMap<TOPIC_GIMBAL_ANGLES>::type));
	_vehicle_data_mutex.unlock();
}
void
FlightCore::getGimbalStatus(TypeMap<TOPIC_GIMBAL_STATUS>::type* gimbal_status){
	_vehicle_data_mutex.lock();
	memcpy(gimbal_status,&_gimbal_status,sizeof(TypeMap<TOPIC_GIMBAL_STATUS>::type));
	_vehicle_data_mutex.unlock();
}
float 
FlightCore::getVehicleBearing(){
	float current_head=	toEulerAngle(static_cast<void*>(&_quaternion)).z*RAD2DEG; //rad-->deg
	return current_head;
}

bool FlightCore::djiArmMotor(){
	char func[50];
	ACK::ErrorCode cmd_status = _vehicle->control->armMotors(1);
	if(ACK::getError(cmd_status) != ACK::SUCCESS){
		ACK::getErrorCodeMessage(cmd_status,func);
		std::string errmsg(func);
		DWAR(__FILE__,__LINE__,"Send arm motor cmd err:"+errmsg,SocketPrintFd);	
		return false;
	}
	return true;
}
bool FlightCore::djiDisarmMotor(){
	char func[50];
	ACK::ErrorCode cmd_status = _vehicle->control->disArmMotors(1);
	if(ACK::getError(cmd_status) != ACK::SUCCESS){
		ACK::getErrorCodeMessage(cmd_status,func);
		std::string errmsg(func);
		DWAR(__FILE__,__LINE__,"Send disarm motor cmd err:"+errmsg,SocketPrintFd);	
		return false;
	}
	return true;
}
bool
FlightCore::djiShootPhoto(){
	if(_gimbal_status.mountStatus == 0){
		DWAR(__FILE__,__LINE__,"Camera not mount.",SocketPrintFd);		
		return false;
	}		
	_vehicle->camera->shootPhoto();
	sleep(1);// waiting 1s for action
	return true;
}
bool 
FlightCore::djiVideoStart(){
	if(_gimbal_status.mountStatus == 0){
		DWAR(__FILE__,__LINE__,"Camera not mount.",SocketPrintFd);		
		return false;
	}	
	_vehicle->camera->videoStart();
	sleep(1);// waiting 2s for action
	return true;
}
bool 
FlightCore::djiVideoStop(){
	if(_gimbal_status.mountStatus == 0){
		DWAR(__FILE__,__LINE__,"Camera not mount.",SocketPrintFd);		
		return false;
	}
	_vehicle->camera->videoStop();
	sleep(1);// waiting 2s for action
	return true;
}
/*
*	zoom the z30 camera by set zoom times 
*	param: input times, range: (1~30)*100=(100~3000)
*
*/
bool
FlightCore::djiCameraZoomByPos(uint16_t times){
	return djiCameraZoom(1,times);
}
bool
FlightCore::djiCameraZoomBySpeed(int16_t speed){
	return djiCameraZoom(2,speed);
}
bool
FlightCore::djiCameraZoomBystep(int16_t times){
	return djiCameraZoom(0,times);
}
bool	
FlightCore::djiCameraZoom(uint16_t mode, int16_t value){
	if(_gimbal_status.mountStatus == 0){
		DWAR(__FILE__,__LINE__,"Camera not mount.",SocketPrintFd);
		return false;
	}
	camera_zoom_data_type_t zoom_data;
	memset(&zoom_data,0,sizeof(camera_zoom_data_type_t));
	zoom_data.func_index = 19;
	zoom_data.cam_index = 1;
	zoom_data.zoom_config.optical_zoom_mode = mode;
	zoom_data.zoom_config.optical_zoom_enable = 1;
	if(mode ==0){
		zoom_data.optical_zoom_param.step_param.zoom_step_level = (uint16_t)abs(value);
		zoom_data.optical_zoom_param.step_param.zoom_step_direction = value>0? 1: 0;
	}else if(mode ==1){
		zoom_data.optical_zoom_param.pos_param.zoom_pos_level = (uint16_t)(abs(value)<100? 100: abs(value)>3000? 3000 : abs(value));
	}else if(mode ==2){
		zoom_data.optical_zoom_param.cont_param.zoom_cont_speed = (uint16_t)abs(value);
		zoom_data.optical_zoom_param.cont_param.zoom_cont_direction = value>0? 1: 0;
	}else{
		DWAR(__FILE__,__LINE__,"Zoom mode err.",SocketPrintFd);
		return false;	
	}
	djiCameraZoom(&zoom_data);
	sleep(1);
	return true;
}
bool	
FlightCore::djiCameraZoom(const camera_zoom_data_type_t *zoom){
	const uint8_t z30_zoom_cmd[] = {0x01, 0x30};
	_vehicle->protocolLayer->send(2, _vehicle->getEncryption(),z30_zoom_cmd,(unsigned char*)zoom, sizeof(camera_zoom_data_type_t), 500, 2);
	return true;
}
bool
FlightCore::djiSetGimbalAngle(float roll_deg,float pitch_deg,float yaw_deg){
	if(_gimbal_status.mountStatus == 0){
		DWAR(__FILE__,__LINE__,"Gimbal not mount.",SocketPrintFd);
		return false;
	}
	
	DJI::OSDK::Gimbal::AngleData target_angle;
	target_angle.roll=(int16_t)((roll_deg)*10);  // deg-->deg*10
	target_angle.pitch=(int16_t)((pitch_deg)*10);
	target_angle.yaw=(int16_t)((yaw_deg)*10); // related to head
	target_angle.duration=10;
	target_angle.mode=1;
	_vehicle->gimbal->setAngle(&target_angle);
	
	double vehicle_head=toEulerAngle(static_cast<void*>(&_quaternion)).z*RAD2DEG;
	// follow mode, so shuld get current head 
	float target_gimbal_yaw=vehicle_head+yaw_deg;
	if(target_gimbal_yaw<-180){
		target_gimbal_yaw += 360;
	}else if (target_gimbal_yaw>180){
		target_gimbal_yaw -= 360;
	}
	int startcount=0;
	while(startcount<5){
		if(fabs(_gimbal_angle.y-roll_deg)<0.5&&
		   fabs(_gimbal_angle.x-pitch_deg)<0.5&&
		   fabs(_gimbal_angle.z-target_gimbal_yaw)<0.5){
			break;
		}
		startcount++;
		sleep(1);// waiting 5*1=5s for gimbal reache the target angle
	}
	if(startcount==5){
		DWAR(__FILE__,__LINE__,"gimbal control Timeout.",SocketPrintFd);		
	}
	return true;
}
bool	
FlightCore::djiSetGImbalSpeed(float roll_rate,float pitch_rate,float yaw_rate){
	if(_gimbal_status.mountStatus == 0){
		DWAR(__FILE__,__LINE__,"Gimbal not mount.",SocketPrintFd);		
		return false;
	}	
	DJI::OSDK::Gimbal::SpeedData gimbal_speed;
	gimbal_speed.roll=(int16_t)roll_rate*10;
	gimbal_speed.pitch=(int16_t)pitch_rate*10;
	gimbal_speed.yaw=(int16_t)yaw_rate*10;
	gimbal_speed.ignore_aircraft_motion=1;
	_vehicle->gimbal->setSpeed(&gimbal_speed);
	return true;
}
