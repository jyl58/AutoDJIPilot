/*
* @file FlightCore.cpp
* @author: JYL
* @email:jiyingliang369@126.com
* @date: 2019.06.10
*
*/
#include <iostream>
#include <cmath>
#include "FlightCore.h"
#include "geo.h"
#include "dji_control.hpp"
#include "Message.h"
#include "DJI_MAVlinkBrige.h"
//init static var
bool FlightCore::_auto_running_need_break=false;
DJI::OSDK::Vehicle* FlightCore::_vehicle=nullptr;

FlightCore::FlightCore()
:_thread_need_exit(false),
_vehicle_rtk_avilable(false)
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
	if(_vehicle_data_mutex != nullptr){
		delete _vehicle_data_mutex;
		_vehicle_data_mutex=nullptr;
	}
	// release ctr authority
	djiReleaseControlAuthority();
}
bool
FlightCore::djiGetControlAuthority(){
	int functionTimeout=1;
	char func[50];
	if(_vehicle == nullptr){
		DWAR("Vehicle pointer is nullptr.");	
		return false;
	}
	//get control authority
	ACK::ErrorCode Status=_vehicle->obtainCtrlAuthority(functionTimeout);
	if (ACK::getError(Status) != ACK::SUCCESS){
		ACK::getErrorCodeMessage(Status,func);
		std::string errmsg(func);
		DWAR("Get control authority err: "+errmsg);	
		return false;
	}
	return true;
}
bool
FlightCore::djiReleaseControlAuthority(){	
	int functionTimeout=1;
	char func[50];
	if(_vehicle == nullptr){
		DWAR("Vehicle pointer is nullptr.");	
		return false;
	}
	//Release control authority
	ACK::ErrorCode Status=_vehicle->releaseCtrlAuthority(functionTimeout);
	if (ACK::getError(Status) != ACK::SUCCESS){
		ACK::getErrorCodeMessage(Status,func);
		std::string errmsg(func);
		DWAR("Release control authority err: "+errmsg);	
		return false;
	}
	return true;
}
bool FlightCore::flightCoreInit(DJI::OSDK::Vehicle *vehicle){
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
		DWAR("Subscribe Verify err: "+errmsg);
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
		DWAR("Init pkgindex 0 err.");
		return pkgStatus;
	}
	subcribeStatus			=_vehicle->subscribe->startPackage(pkgIndex,responseTimeout);
	if (ACK::getError(subcribeStatus) != ACK::SUCCESS){
		ACK::getErrorCodeMessage(subcribeStatus,func);
		_vehicle->subscribe->removePackage(pkgIndex,responseTimeout);
		std::string errmsg(func);
		DWAR("Subscribe Start Package 0 err: "+errmsg);	
		return false;
	}
	//package 1: lat/lon/alt and Velocity at 10 hz
	pkgIndex				=1;
	freq 					=10;
	TopicName topicList10Hz[]	={ TOPIC_GPS_FUSED, TOPIC_HEIGHT_FUSION,TOPIC_GIMBAL_ANGLES,TOPIC_GIMBAL_STATUS,TOPIC_GIMBAL_CONTROL_MODE};
	numTopic				=sizeof(topicList10Hz)/sizeof(topicList10Hz[0]);
	enableTimestamp			=false;
	pkgStatus				=_vehicle->subscribe->initPackageFromTopicList(pkgIndex,numTopic,topicList10Hz,enableTimestamp,freq);
	if(!pkgStatus){
		DWAR("Init pkgindex 1 err.");
		return pkgStatus;
	}
	subcribeStatus			=_vehicle->subscribe->startPackage(pkgIndex,responseTimeout);
	if (ACK::getError(subcribeStatus) != ACK::SUCCESS){
		ACK::getErrorCodeMessage(subcribeStatus, func);
		_vehicle->subscribe->removePackage(pkgIndex,responseTimeout);
		std::string errmsg(func);
		DWAR("Subscribe Start Package 1 err: "+errmsg);	
		return false;
	}
	/*register a callback for global pos mavlink route msg*/
	_vehicle->subscribe->registerUserPackageUnpackCallback(1,GlobalPosCallback);

	//package 2: RC channel and fusion Velocity at 20 hz
	pkgIndex				=2;
	freq 					=20;
	TopicName topicList20Hz[]	={ TOPIC_RC_FULL_RAW_DATA, TOPIC_VELOCITY,TOPIC_GPS_SIGNAL_LEVEL,TOPIC_RC,TOPIC_RC_WITH_FLAG_DATA};
	numTopic				=sizeof(topicList20Hz)/sizeof(topicList20Hz[0]);
	enableTimestamp			=false;
	pkgStatus				=_vehicle->subscribe->initPackageFromTopicList(pkgIndex,numTopic,topicList20Hz,enableTimestamp,freq);
	if(!pkgStatus){
		DWAR("Init pkgindex 2 err.");
		return pkgStatus;
	}
	subcribeStatus			=_vehicle->subscribe->startPackage(pkgIndex,responseTimeout);
	if (ACK::getError(subcribeStatus) != ACK::SUCCESS){
		ACK::getErrorCodeMessage(subcribeStatus, func);
		_vehicle->subscribe->removePackage(pkgIndex,responseTimeout);
		std::string errmsg(func);
		DWAR("Subscribe Start Package 2 err: "+errmsg);	
		return false;
	}
	_vehicle->subscribe->registerUserPackageUnpackCallback(2,RCCallback);

	//package 3: Quaternion  at 200 hz
	pkgIndex					=3;
	freq 						=200;
	TopicName topicList200Hz[]	={ TOPIC_QUATERNION };
	numTopic					=sizeof(topicList200Hz)/sizeof(topicList200Hz[0]);
	enableTimestamp				=false;
	pkgStatus					=_vehicle->subscribe->initPackageFromTopicList(pkgIndex,numTopic,topicList200Hz,enableTimestamp,freq);
	if(!pkgStatus){
		DWAR("Init pkgindex 3 err.");
		return pkgStatus;
	}
	subcribeStatus				=_vehicle->subscribe->startPackage(pkgIndex,responseTimeout);
	if (ACK::getError(subcribeStatus) != ACK::SUCCESS){
		ACK::getErrorCodeMessage(subcribeStatus, func);
		_vehicle->subscribe->removePackage(pkgIndex,responseTimeout);
		std::string errmsg(func);
		DWAR("Subscribe Start Package 3 err: "+errmsg);	
		return false;
	}
	
	//package 4: RTK  at 5 hz
	pkgIndex					=4;
	freq 						=5;
	TopicName topicListRTK5Hz[]	={ TOPIC_RTK_POSITION, TOPIC_RTK_YAW_INFO, TOPIC_RTK_POSITION_INFO,TOPIC_RTK_VELOCITY,TOPIC_RTK_YAW};
	numTopic					=sizeof(topicListRTK5Hz)/sizeof(topicListRTK5Hz[0]);
	enableTimestamp				=false;
	pkgStatus					=_vehicle->subscribe->initPackageFromTopicList(pkgIndex,numTopic,topicListRTK5Hz,enableTimestamp,freq);
	if(!pkgStatus){
		DWAR("Init pkgindex 4 err.");
		return pkgStatus;
	}
	subcribeStatus				=_vehicle->subscribe->startPackage(pkgIndex,responseTimeout);
	if(subcribeStatus.data == ErrorCode::SubscribeACK::SOURCE_DEVICE_OFFLINE){
		DWAR("RTK Not Available");
		_vehicle_rtk_avilable=false;
	}else {
		_vehicle_rtk_avilable=true;
		if(ACK::getError(subcribeStatus) != ACK::SUCCESS){
			ACK::getErrorCodeMessage(subcribeStatus,func);
			_vehicle->subscribe->removePackage(pkgIndex,responseTimeout);
			std::string errmsg(func);
			DWAR("Subscribe Start Package 4 err: "+errmsg);	
			return false;
		}
	}

	exitDjiThread();
	//creat a new mutex 
	_vehicle_data_mutex=new std::mutex();
	//creat a new thread for swap data for FC
	_dji_FC_link_thread=new std::thread(&FlightCore::readVehicleStatusThread,this);
	return true;
}

void FlightCore::readVehicleStatusThread(){
	FLIGHTLOG("Start run flight core thread");
	while(!_thread_need_exit){
		//lock the mutex 
		_vehicle_data_mutex->lock();
		//flight status
		_flightStatus 		= _vehicle->subscribe->getValue<TOPIC_STATUS_FLIGHT>();
		_display_mode		= _vehicle->subscribe->getValue<TOPIC_STATUS_DISPLAYMODE>(); 
		_battery_info		= _vehicle->subscribe->getValue<TOPIC_BATTERY_INFO>();
		_gps_signal_level	= _vehicle->subscribe->getValue<TOPIC_GPS_SIGNAL_LEVEL>();
		// lat lon 
		_current_lat_lon	= _vehicle->subscribe->getValue<TOPIC_GPS_FUSED>();
		_height_fusioned	= _vehicle->subscribe->getValue<TOPIC_HEIGHT_FUSION>();
		_gps_details		= _vehicle->subscribe->getValue<TOPIC_GPS_DETAILS>();
		_rc_full_raw_data	= _vehicle->subscribe->getValue<TOPIC_RC_FULL_RAW_DATA>();
		_rc_data			=_vehicle->subscribe->getValue<TOPIC_RC>();
		_velocity	  		= _vehicle->subscribe->getValue<TOPIC_VELOCITY>();
		_quaternion	  		= _vehicle->subscribe->getValue<TOPIC_QUATERNION>();
		_gimbal_status		=_vehicle->subscribe->getValue<TOPIC_GIMBAL_STATUS>();
		if(_gimbal_status.mountStatus){
			_gimbal_angle		=_vehicle->subscribe->getValue<TOPIC_GIMBAL_ANGLES>();
			_gimbal_mode		=_vehicle->subscribe->getValue<TOPIC_GIMBAL_CONTROL_MODE>();
		}
		if(_vehicle_rtk_avilable){
			_rtk_pos	  		= _vehicle->subscribe->getValue<TOPIC_RTK_POSITION>();
			_rtk_pos_info	  	= _vehicle->subscribe->getValue<TOPIC_RTK_POSITION_INFO>();
			_rtk_velocity	  	= _vehicle->subscribe->getValue<TOPIC_RTK_VELOCITY>();
			_rtk_yaw	  		= _vehicle->subscribe->getValue<TOPIC_RTK_YAW>();
			_rtk_yaw_info	  	= _vehicle->subscribe->getValue<TOPIC_RTK_YAW_INFO>();
		}
		//unlock the mutex		
		_vehicle_data_mutex->unlock();
		usleep(5000); //sleep 5 ms
	}
	FLIGHTLOG("Exit flight core thread");
	//remove the subscribe the package	
	for (int i=0;i<MAX_PKG_COUNT;i++){
		_vehicle->subscribe->removePackage(i,1);
	}
}
void 
FlightCore::getFlightStatus(TypeMap<TOPIC_STATUS_FLIGHT>::type* flightStatus){
	_vehicle_data_mutex->lock();
	memcpy(flightStatus,&_flightStatus,sizeof(TypeMap<TOPIC_STATUS_FLIGHT>::type));
	_vehicle_data_mutex->unlock();
}
void 
FlightCore::getFlightBatteryInfo(TypeMap<TOPIC_BATTERY_INFO>::type* battery_info){
	_vehicle_data_mutex->lock();
	memcpy(battery_info,&_battery_info,sizeof(TypeMap<TOPIC_BATTERY_INFO>::type));
	_vehicle_data_mutex->unlock();
}
void 
FlightCore::getVehicleGPS(TypeMap<TOPIC_GPS_FUSED>::type* lat_lon){
	_vehicle_data_mutex->lock();
	memcpy(lat_lon,&_current_lat_lon,sizeof(TypeMap<TOPIC_GPS_FUSED>::type));
	_vehicle_data_mutex->unlock();
}
void 
FlightCore::getVehicleGpsSignalLevel(TypeMap<TOPIC_GPS_SIGNAL_LEVEL>::type* gps_signal){
	_vehicle_data_mutex->lock();
	memcpy(gps_signal,&_gps_signal_level,sizeof(TypeMap<TOPIC_GPS_SIGNAL_LEVEL>::type));
	_vehicle_data_mutex->unlock();
}
void
FlightCore::getVehicleAltitude(TypeMap<TOPIC_HEIGHT_FUSION>::type* height){
	_vehicle_data_mutex->lock();
	memcpy(height,&_height_fusioned,sizeof(TypeMap<TOPIC_HEIGHT_FUSION>::type));
	_vehicle_data_mutex->unlock();
}
void 
FlightCore::getVehicleGpsDetails(TypeMap<TOPIC_GPS_DETAILS>::type* gps_details){
	_vehicle_data_mutex->lock();
	memcpy(gps_details,&_gps_details,sizeof(TypeMap<TOPIC_GPS_DETAILS>::type));
	_vehicle_data_mutex->unlock();
}
void 
FlightCore::getVehicleVelocity(TypeMap<TOPIC_VELOCITY>::type* velocity){
	_vehicle_data_mutex->lock();
	memcpy(velocity,&_velocity,sizeof(TypeMap<TOPIC_VELOCITY>::type));
	_vehicle_data_mutex->unlock();
}
void 
FlightCore::getVehicleQuaternion(TypeMap<TOPIC_QUATERNION>::type* quaternion){
	_vehicle_data_mutex->lock();
	memcpy(quaternion,&_quaternion,sizeof(TypeMap<TOPIC_QUATERNION>::type));
	_vehicle_data_mutex->unlock();
}
void 
FlightCore::getVehicleDisplay(TypeMap<TOPIC_STATUS_DISPLAYMODE>::type*	display_mode){
	_vehicle_data_mutex->lock();
	memcpy(display_mode,&_display_mode,sizeof(TypeMap<TOPIC_STATUS_DISPLAYMODE>::type));
	_vehicle_data_mutex->unlock();
}
void	
FlightCore::getGimbalAngle(TypeMap<TOPIC_GIMBAL_ANGLES>::type	*gimbal_angle){
	_vehicle_data_mutex->lock();
	memcpy(gimbal_angle,&_gimbal_angle,sizeof(TypeMap<TOPIC_GIMBAL_ANGLES>::type));
	_vehicle_data_mutex->unlock();
}
void
FlightCore::getGimbalStatus(TypeMap<TOPIC_GIMBAL_STATUS>::type* gimbal_status){
	_vehicle_data_mutex->lock();
	memcpy(gimbal_status,&_gimbal_status,sizeof(TypeMap<TOPIC_GIMBAL_STATUS>::type));
	_vehicle_data_mutex->unlock();
}
float 
FlightCore::getVehicleBearing(){
	float current_head=	toEulerAngle(static_cast<void*>(&_quaternion)).z*RAD2DEG; //rad-->deg
	return current_head;
}
/*@do a takeoff */
bool 
FlightCore::djiTakeoff(){
	if(_flightStatus == VehicleStatus::FlightStatus::IN_AIR){
		DWAR("Running takeoff, but the vehicle is in air already");
		return true;
	}
	if(_gps_signal_level<3){
		DWAR("Takeoff need GPS signal level >=3.");		
		return false;
	}

	int timeout=1;
	char func[50];
	// send takeoff cmd
	ACK::ErrorCode takeoffstatus = _vehicle->control->takeoff(timeout);
	if(ACK::getError(takeoffstatus) != ACK::SUCCESS){
		ACK::getErrorCodeMessage(takeoffstatus,func);
		std::string errmsg(func);
		DWAR("Send takeoff CMD err:"+errmsg);		
		return false;
	}
	/*First check: motor is start*/
	if(!_vehicle->isM100() && !_vehicle->isLegacyM600()){
		int cmdstart=0;	
		while(_flightStatus != VehicleStatus::FlightStatus::ON_GROUND && 
			  _display_mode != VehicleStatus::DisplayMode::MODE_ENGINE_START&& 
			  cmdstart<20){		
			cmdstart++;
			usleep(100000); //waiting 20*100ms=2s
		}
		if(cmdstart == 20){
			DWAR("Takeoff filed, Motors are not spining");		
			return false;
		}
	}else{
	//TODO: add m600 and m100 process code
	}
	
	/*Second check: vehicle is in air*/
	if(!_vehicle->isM100() && !_vehicle->isLegacyM600()){
		int onground_count=0;	
		//waitting 10 s	
		while(_flightStatus != VehicleStatus::FlightStatus::IN_AIR &&
			  (_display_mode != VehicleStatus::DisplayMode::MODE_ASSISTED_TAKEOFF || _display_mode != VehicleStatus::DisplayMode::MODE_AUTO_TAKEOFF)&& 
			  onground_count<100 ){		
			onground_count++;
			usleep(100000);
		}
		if(onground_count == 100){
			DWAR("Takeoff filed, Vehicle is still on the ground,but motors is spin ");		
			return false;
		}
	}else{
	//TODO: add m600 and m100 process code
	}

	/*Third check : takeoff finished*/ 
	if(!_vehicle->isM100() && !_vehicle->isLegacyM600()){
		while(_display_mode == VehicleStatus::DisplayMode::MODE_ASSISTED_TAKEOFF ||
			  _display_mode == VehicleStatus::DisplayMode::MODE_AUTO_TAKEOFF){		
			sleep(1);
		}
		if(_display_mode != VehicleStatus::DisplayMode::MODE_P_GPS||
		   _display_mode != VehicleStatus::DisplayMode::MODE_ATTITUDE){
			//std::cout<<"Takeoff successful"<<"\n"<<std::endl;
		}else{
			DWAR("Takeoff finished,but the vehicle is in an unexpected mode");
			return false;			
		}
	}else{
	//TODO: add m600 and m100 process code
	}
	return true;
}
bool FlightCore::djiLanding(){
	if(_flightStatus == VehicleStatus::FlightStatus::ON_GROUND){
		DWAR("Running landing ,but the the vehicle is on ground already");	
		return true;
	}
	int timeout=1;
	char func[50];
	ACK::ErrorCode landingStatus=_vehicle->control->land(timeout);
	if(ACK::getError(landingStatus) != ACK::SUCCESS){
		ACK::getErrorCodeMessage(landingStatus,func);
		std::string errmsg(func);
		DWAR("Send landing CMD err:"+errmsg);		
		return false;	
	}
	//first check landing stared
	if(!_vehicle->isM100() && !_vehicle->isLegacyM600()){
		int landNoStart=0;
		while(_display_mode !=  VehicleStatus::DisplayMode::MODE_AUTO_LANDING&&
			  landNoStart<20){
			landNoStart++;
			usleep(100000); // 20*100000 waiting 2s
		}
		if(landNoStart==20){
			return false;
		}
		//second check: finished landing
		while(_display_mode == VehicleStatus::DisplayMode::MODE_AUTO_LANDING &&
			  _flightStatus == VehicleStatus::FlightStatus::IN_AIR){
				sleep(1);// waitting vehicle landed
		}
		if(_display_mode != VehicleStatus::DisplayMode::MODE_P_GPS||
		   _display_mode != VehicleStatus::DisplayMode::MODE_ATTITUDE){
			//std::cout<<"Landing successful"<<"\n"<<std::endl;
		}
	}else{
		//TODO: add m100 and m600 process
	}
	return true;
}
bool FlightCore::djiGoHome(){
	if(_flightStatus != VehicleStatus::FlightStatus::IN_AIR){
		DWAR("Running go home, but the vehicle is not in air.");	
		return false;
	}
	int timeout=1;
	char func[50];
	ACK::ErrorCode goHomeStatus= _vehicle->control->goHome(timeout);
	if(ACK::getError(goHomeStatus) != ACK::SUCCESS){
		ACK::getErrorCodeMessage(goHomeStatus,func);
		std::string errmsg(func);
		DWAR("send go home CMD err:"+errmsg);
		return false;
	}
	//first check: start go home
	if(!_vehicle->isM100() && !_vehicle->isLegacyM600()){
		int startGohome=0;		
		while(_display_mode != VehicleStatus::DisplayMode::MODE_NAVI_GO_HOME&&
			  startGohome<20){
			 startGohome++;	
			 usleep(100000); // 20*100000 waiting 2s	
		}
		if(startGohome == 20){
			return false;		
		}
		//second check: finished go home
		while(_display_mode == VehicleStatus::DisplayMode::MODE_NAVI_GO_HOME &&
			  _flightStatus == VehicleStatus::FlightStatus::IN_AIR){
				sleep(1);// waitting vehicle go home
		}
	}else{
		//TODO: add m100 and m600 process
	}
	return true;
}
/*
*	move vehicle by z offset. frame:NEU
*	param target_alt: altitude offset. unit:m
*/
bool FlightCore::djiMoveZByOffset(float target_alt_m,float vertical_threshold_in_m){
	if(_flightStatus != VehicleStatus::FlightStatus::IN_AIR){
		DWAR("Running move by z offset,but the vehicle is not in air.");	
		return false;
	}
	TypeMap<TOPIC_HEIGHT_FUSION>::type 	task_startAltitude=_height_fusioned;
	if(!_vehicle->isM100() && !_vehicle->isLegacyM600()){	
		float xcmd=0,ycmd=0,zcmd=0;
		float yaw_in_rad=toEulerAngle(static_cast<void*>(&_quaternion)).z;
		zcmd =task_startAltitude + target_alt_m;
		// FLY MODE flags
		uint8_t ctrl_flag = Control::VERTICAL_POSITION;
		while(!_auto_running_need_break){
				Control::CtrlData data(ctrl_flag,xcmd,ycmd,zcmd,yaw_in_rad*RAD2DEG);
				_vehicle->control->flightCtrl(data);
				usleep(20000);  //20ms
				float z_offset_remaing=_height_fusioned-task_startAltitude;
				//check the alt is reache 
				if(std::fabs(target_alt_m - z_offset_remaing)< vertical_threshold_in_m)
					break;
		}
		if(_auto_running_need_break){
			_auto_running_need_break=false;
			FLIGHTLOG("Move Z by offset is breaked.");			
			return false;	
		}
	}else{
	//TODO: add m100 and m600 process
	}
	return true;
}
bool FlightCore::djiMoveZToTarget(float target_alt_m){
	if(_flightStatus != VehicleStatus::FlightStatus::IN_AIR){
		DWAR("Running move to z target, but the vehicle is not in air.");	
		return false;
	}
	return djiMoveZByOffset(target_alt_m-_height_fusioned);
}
/*
*	move vehicle by target GPS point at current altitude
*	param target_lat_deg: target point GPS latitude. unit: deg
*	param target_lon_deg: target point GPS longitude. unit: deg
*/
bool FlightCore::djiMoveByGPS(double target_lat_deg,double target_lon_deg){
	if(_flightStatus != VehicleStatus::FlightStatus::IN_AIR){
		DWAR("Running move by gps,but the vehicle is not in air.");	
		return false;
	}
	float x_offset=0;
	float y_offset=0;
	//get the target point NE vector for current point
	_get_vector_to_next_waypoint(_current_lat_lon.latitude*RAD2DEG, _current_lat_lon.longitude*RAD2DEG, target_lat_deg, target_lon_deg, &x_offset, &y_offset);
	
	return djiMoveX_YByOffset(x_offset,y_offset);
}
/* 
*	move vehicle by the x and y offset in same alt frame:NEU
*	parm target_x_m,target_y_m: horizontal offset 
*	param pos_threshold_in_m: threshold for stop point
*/
bool FlightCore::djiMoveX_YByOffset(float target_x_m, float target_y_m, float pos_threshold_in_m){
	if(_flightStatus != VehicleStatus::FlightStatus::IN_AIR){
		DWAR("Runing move by x_y offset,but vehicle is not in air.");	
		return false;
	}
	//record the start point
	TypeMap<TOPIC_GPS_FUSED>::type task_startPoint=_current_lat_lon;
	TypeMap<TOPIC_GPS_FUSED>::type current_lat_lon;
	
	if(!_vehicle->isM100() && !_vehicle->isLegacyM600()){
		Telemetry::Vector3f localoffset;
		float x_speed_factor=0;
		float y_speed_factor=0;
		// FLY MODE flags
		uint8_t ctrl_flag = Control::HORIZONTAL_POSITION | Control::HORIZONTAL_GROUND | Control::VERTICAL_POSITION |Control::YAW_ANGLE | Control::STABLE_ENABLE;		
		do{
			//get the new local pos
			getVehicleGPS(&current_lat_lon);
			//
			localOffsetFromGPSOffset(localoffset, static_cast<void*>(&current_lat_lon), static_cast<void*>(&task_startPoint));
			//get the initial offset we will update in loop		
			float x_offset_remaing=target_x_m-localoffset.x;
			float y_offset_remaing=target_y_m-localoffset.y;
			
			//check reached the threshold 
			if((std::fabs(x_offset_remaing)< pos_threshold_in_m ) && 
			   (std::fabs(y_offset_remaing)< pos_threshold_in_m)){
				break;
			}
			//flight interrupt by ext
			if(_auto_running_need_break){
				_auto_running_need_break=false;
				FLIGHTLOG("Move X_Y by offset is breaked.");			
				return false;	
			}
			//distance to target point,
			float distance = sqrtf(powf(x_offset_remaing,2)+powf(y_offset_remaing,2));
			float v_factor = distance > BREAK_BOUNDARY? MAX_SPEED_FACTOR : distance/BREAK_BOUNDARY;
			//limit the min value
			if(v_factor < MIN_SPEED_FACTOR){
				v_factor=MIN_SPEED_FACTOR;
			}
			if(distance>0.01){
				x_speed_factor=v_factor*(x_offset_remaing/distance);
				y_speed_factor=v_factor*(y_offset_remaing/distance);
			}else{
				x_speed_factor=0;
				y_speed_factor=0;
			}
			float xcmd=x_speed_factor;
			float ycmd=y_speed_factor;
			float zcmd=_height_fusioned; // at same height fly
			float yaw_in_rad=toEulerAngle(static_cast<void*>(&_quaternion)).z; // hold the head 

			Control::CtrlData data(ctrl_flag,xcmd,ycmd,zcmd,yaw_in_rad*RAD2DEG);
			_vehicle->control->flightCtrl(data);
			usleep(20000);  //20ms
		}while(!_auto_running_need_break);
		
		// do a emergencyBrake 
		_vehicle->control->emergencyBrake();
	}else{
		//TODO: add m100 and m600
	}
	return true;
}
bool	
FlightCore::djiMoveByBearingAndDistance(float bearing,float distance){
	if(_flightStatus != VehicleStatus::FlightStatus::IN_AIR){
		DWAR("Run reletive fly ,but the vehicle is not in air.");	
		return false;
	}
	double lat;
	double lon;
	_waypoint_from_heading_and_distance(_current_lat_lon.latitude*RAD2DEG,_current_lat_lon.longitude*RAD2DEG,bearing*DEG2RAD,distance,&lat,&lon);
	return djiMoveByGPS(lat,lon);
}
/*
*	move vehicle by velocity,need call this function in loop untill finished fly.frame:NEU
*	param :vx,vy,vz: m/s
*/
bool  FlightCore::djiMoveByVelocity(float vx,float vy,float vz){
	if(_flightStatus != VehicleStatus::FlightStatus::IN_AIR){
		DWAR("Run velocity fly ,but the vehicle is not in air.");	
		return false;
	}
	if(!_vehicle->isM100() && !_vehicle->isLegacyM600()){
		_vehicle->control->velocityAndYawRateCtrl(vx,vy,vz,0);
	}
	return true;
}
/*
*	Turn Vehicle head to target value
*	param target_head_deg: target head. uint: deg
*	param yaw_threshold_in_deg: threshold value. default:1 deg
*/
bool FlightCore::djiTurnHead(float target_head_deg,float yaw_threshold_in_deg){
	if(_flightStatus != VehicleStatus::FlightStatus::IN_AIR){
		DWAR("Run turn head,but  the vehicle is not in air.");	
		return false;
	}
	if(!_vehicle->isM100() && !_vehicle->isLegacyM600()){
		float xcmd=0,ycmd=0;
		float zcmd=_height_fusioned;
		// FLY MODE flags <note>: this need z control or alt maybe change ,because turn head 
		uint8_t ctrl_flag = Control::YAW_ANGLE | Control::VERTICAL_POSITION;
		while(!_auto_running_need_break){
			Control::CtrlData data(ctrl_flag,xcmd,ycmd,zcmd,target_head_deg);
			_vehicle->control->flightCtrl(data);
			usleep(20000);  //20ms
			float current_yaw_in_rad=toEulerAngle(static_cast<void*>(&_quaternion)).z;	
			//check if finished turn head
			if(std::fabs(current_yaw_in_rad*RAD2DEG-target_head_deg) < yaw_threshold_in_deg)
				break;	
		}
		if(_auto_running_need_break){
			_auto_running_need_break=false;
			FLIGHTLOG("Trun head is breaked.");			
			return false;	
		}
	}else{
		//TODO: add m100 and m600
	}
	return true;
}
/*
*	move vehicle by offset pos and yaw 
*	control vehicle fly to target point and target head
*	param: x_offset_Desired,y_offset_Desired,z_offset_Desired: uint: m
*	param: yaw_Desired:unit: deg
*	param: pos_threshold_in_m,the threshold of target point,when in the threshold we stop control vehile fly. unit: m
*	param: yaw_threshold_in_deg,the yaw threshold. unit:deg
*/
bool FlightCore::djiMoveByPosOffset(float x_offset_Desired,float y_offset_Desired ,float z_offset_Desired, float yaw_Desired,float pos_threshold_in_m,float yaw_threshold_in_deg){
	
	if(_flightStatus != VehicleStatus::FlightStatus::IN_AIR){
		DWAR("Run the move by pos offset,but vehicle is not in air");	
		return false;
	}
	Telemetry::Vector3f localoffset;
	//record the start point
	TypeMap<TOPIC_GPS_FUSED>::type 		task_startPoint		=_current_lat_lon;
	TypeMap<TOPIC_HEIGHT_FUSION>::type 	task_startAltitude	=_height_fusioned;
	if(!_vehicle->isM100() && !_vehicle->isLegacyM600()){
		float x_speed_factor=0;
		float y_speed_factor=0;
		float z_speed_factor=0;
		double yaw_threshold_in_rad	=	DEG2RAD * yaw_threshold_in_deg;
		double yaw_desired_rad      =   DEG2RAD * yaw_Desired;
		do{ 		
			localOffsetFromGPSOffset(localoffset, static_cast<void*>(&_current_lat_lon), static_cast<void*>(&task_startPoint));
			localoffset.z=_height_fusioned-task_startAltitude;
			//get the initial offset we will update in loop		
			double x_offset_remaing=x_offset_Desired-localoffset.x;
			double y_offset_remaing=y_offset_Desired-localoffset.y;
			double z_offset_remaing=z_offset_Desired-localoffset.z;
			//current the head		
			double yaw_in_rad=toEulerAngle(static_cast<void*>(&_quaternion)).z;
			//checke if finished control
			if(std::fabs(x_offset_remaing)< pos_threshold_in_m &&
			   std::fabs(y_offset_remaing)< pos_threshold_in_m &&
			   std::fabs(z_offset_remaing)< 0.5 &&
			   std::fabs(yaw_in_rad-yaw_desired_rad) < yaw_threshold_in_rad){
				break;
			}
			//check interrupt command
			if(_auto_running_need_break){
				_auto_running_need_break=false;
				FLIGHTLOG("Move X,Y,Z,yaw by offset is breaked.");			
				return false;	
			}
			float distance=sqrtf(powf(x_offset_remaing,2) + powf(y_offset_remaing,2) + powf(z_offset_remaing,2));
			float v_factor=distance > BREAK_BOUNDARY? MAX_SPEED_FACTOR : distance/BREAK_BOUNDARY;
			if(v_factor < MIN_SPEED_FACTOR){
				v_factor=MIN_SPEED_FACTOR;
			}
			if(distance > 0.01){
				x_speed_factor=v_factor*(x_offset_remaing/distance);
				y_speed_factor=v_factor*(y_offset_remaing/distance);
				z_speed_factor=v_factor*(z_offset_remaing/distance);
			}else{
				x_speed_factor=0;
				y_speed_factor=0;
				z_speed_factor=0;
			}
			float xcmd=x_speed_factor;
			float ycmd=y_speed_factor;
			float zcmd =_height_fusioned +z_speed_factor;
			
			//send the control cmd to vehicle fc
			_vehicle->control->positionAndYawCtrl(xcmd,ycmd,zcmd,yaw_Desired);			
			usleep(20000);  //20ms
		}while(!_auto_running_need_break);
		// do a emergencyBrake 
		_vehicle->control->emergencyBrake();		
	}else{
		//TODO: add m600 and m100 process code	
	}
	return true;
}
/*
* hover the dji vehicle to current pos
*/
bool
FlightCore::djiHover(){
	if(_flightStatus == VehicleStatus::FlightStatus::IN_AIR){
		//vehicle in air set pos offset 0,0,0 and velocity 0,0,0
		djiMoveByPosOffset(0,0,0,0);
		djiMoveByVelocity(0,0,0);
	}
	return true;
}
bool FlightCore::djiArmMotor(){
	char func[50];
	ACK::ErrorCode cmd_status = _vehicle->control->armMotors(1);
	if(ACK::getError(cmd_status) != ACK::SUCCESS){
		ACK::getErrorCodeMessage(cmd_status,func);
		std::string errmsg(func);
		DWAR("Send arm motor cmd err:"+errmsg);	
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
		DWAR("Send disarm motor cmd err:"+errmsg);	
		return false;
	}
	return true;
}
bool
FlightCore::djiShootPhoto(){
	if(_gimbal_status.mountStatus == 0){
		DWAR("Camera not mount.");		
		return false;
	}		
	_vehicle->camera->shootPhoto();
	sleep(1);// waiting 1s for action
	return true;
}
bool 
FlightCore::djiVideoStart(){
	if(_gimbal_status.mountStatus == 0){
		DWAR("Camera not mount.");		
		return false;
	}	
	_vehicle->camera->videoStart();
	sleep(1);// waiting 2s for action
	return true;
}
bool 
FlightCore::djiVideoStop(){
	if(_gimbal_status.mountStatus == 0){
		DWAR("Camera not mount.");		
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
		DWAR("Camera not mount.");
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
		DWAR("Zoom mode err.");
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
		DWAR("Gimbal not mount.");
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
		DWAR("gimbal control Timeout.");		
	}
	return true;
}
bool	
FlightCore::djiSetGImbalSpeed(float roll_rate,float pitch_rate,float yaw_rate){
	if(_gimbal_status.mountStatus == 0){
		DWAR("Gimbal not mount.");		
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
