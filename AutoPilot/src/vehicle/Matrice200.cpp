#include <iostream>
#include <sstream>
#include <cmath>
#include "LuaParser.h"
#include "dji_control.hpp"
#include "geo.h"
#include "Message.h"
#include "Matrice200.h"
Matrice200::Matrice200():
FlightCore()
{
}
/*@do a takeoff */
bool 
Matrice200::djiTakeoff(){
	if(_flightStatus == VehicleStatus::FlightStatus::IN_AIR){
		DWAR(__FILE__,__LINE__,"Running takeoff, but the vehicle is in air already",SocketPrintFd);
		return true;
	}
	if(_gps_signal_level<3){
		DWAR(__FILE__,__LINE__,"Takeoff need GPS signal level >=3.",SocketPrintFd);		
		return false;
	}
	int timeout=1;
	char func[50];
	// send takeoff cmd
	ACK::ErrorCode takeoffstatus = _vehicle->control->takeoff(timeout);
	if(ACK::getError(takeoffstatus) != ACK::SUCCESS){
		ACK::getErrorCodeMessage(takeoffstatus,func);
		std::string errmsg(func);
		DWAR(__FILE__,__LINE__,"Send takeoff CMD err:"+errmsg,SocketPrintFd);		
		return false;
	}
	//First check: motor is start
	int cmdstart=0;	
	while(_flightStatus != VehicleStatus::FlightStatus::ON_GROUND && 
		 _display_mode != VehicleStatus::DisplayMode::MODE_ENGINE_START&& 
		 cmdstart<30){		
		cmdstart++;
		usleep(100000); //waiting 30*100ms=3s
	}
	if(cmdstart == 20){
		DWAR(__FILE__,__LINE__,"Takeoff filed, Motors are not spining",SocketPrintFd);		
		return false;
	}

	//Second check: vehicle is in air
	int onground_count=0;	
	//waitting 10 s	
	while(_flightStatus != VehicleStatus::FlightStatus::IN_AIR &&
		  (_display_mode != VehicleStatus::DisplayMode::MODE_ASSISTED_TAKEOFF || _display_mode != VehicleStatus::DisplayMode::MODE_AUTO_TAKEOFF)&& 
		onground_count<100 ){		
		onground_count++;
		usleep(100000);
	}
	if(onground_count == 100){
		DWAR(__FILE__,__LINE__,"Takeoff filed, Vehicle is still on the ground,but motors is spin ",SocketPrintFd);		
		return false;
	}

	//Third check : takeoff finished
	while(_display_mode == VehicleStatus::DisplayMode::MODE_ASSISTED_TAKEOFF ||
		  _display_mode == VehicleStatus::DisplayMode::MODE_AUTO_TAKEOFF){		
		sleep(1);
	}
	if(_display_mode != VehicleStatus::DisplayMode::MODE_P_GPS||
	   _display_mode != VehicleStatus::DisplayMode::MODE_ATTITUDE){
			//std::cout<<"Takeoff successful"<<"\n"<<std::endl;
	}else{
		DWAR(__FILE__,__LINE__,"Takeoff finished,but the vehicle is in an unexpected mode",SocketPrintFd);
		return false;			
	}
	return true;
}
bool 
Matrice200::djiLanding(){
	if(_flightStatus == VehicleStatus::FlightStatus::ON_GROUND){
		DWAR(__FILE__,__LINE__,"Running landing ,but the the vehicle is on ground already",SocketPrintFd);	
		return true;
	}
	int timeout=1;
	char func[50];
	ACK::ErrorCode landingStatus=_vehicle->control->land(timeout);
	if(ACK::getError(landingStatus) != ACK::SUCCESS){
		ACK::getErrorCodeMessage(landingStatus,func);
		std::string errmsg(func);
		DWAR(__FILE__,__LINE__,"Send landing CMD err:"+errmsg,SocketPrintFd);		
		return false;	
	}
	//first check landing stared
	
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
	return true;
}
bool 
Matrice200::djiGoHome(){
	if(_flightStatus != VehicleStatus::FlightStatus::IN_AIR){
		DWAR(__FILE__,__LINE__,"Running go home, but the vehicle is not in air.",SocketPrintFd);	
		return false;
	}
	int timeout=1;
	char func[50];
	ACK::ErrorCode goHomeStatus= _vehicle->control->goHome(timeout);
	if(ACK::getError(goHomeStatus) != ACK::SUCCESS){
		ACK::getErrorCodeMessage(goHomeStatus,func);
		std::string errmsg(func);
		DWAR(__FILE__,__LINE__,"send go home CMD err:"+errmsg,SocketPrintFd);
		return false;
	}
	//first check: start go home
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
	return true;
}
/*
*	move vehicle by target GPS point at current altitude
*	param target_lat_deg: target point GPS latitude. unit: deg
*	param target_lon_deg: target point GPS longitude. unit: deg
*/
bool 
Matrice200::djiMoveByGPS(double target_lat_deg,double target_lon_deg){
	if(_flightStatus != VehicleStatus::FlightStatus::IN_AIR){
		DWAR(__FILE__,__LINE__,"Running move by gps,but the vehicle is not in air.",SocketPrintFd);	
		return false;
	}
	float x_offset=0;
	float y_offset=0;
	//get the target point NE vector for current point
	_get_vector_to_next_waypoint(_current_lat_lon.latitude*RAD2DEG, _current_lat_lon.longitude*RAD2DEG, target_lat_deg, target_lon_deg, &x_offset, &y_offset);
	
	return djiMoveX_YByOffset(x_offset,y_offset);
}
bool	
Matrice200::djiMoveByBearingAndDistance(float bearing,float distance){
	if(_flightStatus != VehicleStatus::FlightStatus::IN_AIR){
		DWAR(__FILE__,__LINE__,"Run reletive fly, but the vehicle is not in air.",SocketPrintFd);	
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
bool  
Matrice200::djiMoveByVelocity(float vx,float vy,float vz){
	if(_flightStatus != VehicleStatus::FlightStatus::IN_AIR){
		DWAR(__FILE__,__LINE__,"Run velocity fly ,but the vehicle is not in air.",SocketPrintFd);	
		return false;
	}
	_vehicle->control->velocityAndYawRateCtrl(vx,vy,vz,0);
	return true;
}
/*
*	move vehicle by z offset. frame:NEU
*	param target_alt: altitude offset. unit:m
*/
bool 
Matrice200::djiMoveZByOffset(float target_alt_m,float vertical_threshold_in_m){
	if(_flightStatus != VehicleStatus::FlightStatus::IN_AIR){
		DWAR(__FILE__,__LINE__,"Running move by z offset,but the vehicle is not in air.",SocketPrintFd);	
		return false;
	}
	TypeMap<TOPIC_HEIGHT_FUSION>::type 	task_startAltitude=_height_fusioned;
	float xcmd=0,ycmd=0,zcmd=0;
	float yaw_in_rad=toEulerAngle(static_cast<void*>(&_quaternion)).z;
	zcmd =task_startAltitude + target_alt_m;
	// FLY MODE flags
	uint8_t ctrl_flag = Control::VERTICAL_POSITION;
	while(true){
		Control::CtrlData data(ctrl_flag,xcmd,ycmd,zcmd,yaw_in_rad*RAD2DEG);
		_vehicle->control->flightCtrl(data);
		usleep(20000);  //20ms
		float z_offset_remaing=_height_fusioned-task_startAltitude;
		//check the alt is reache 
		if(std::fabs(target_alt_m - z_offset_remaing)< vertical_threshold_in_m){
			break;
		}
		//run hook function
		LuaParser::runLuaHookFunction();
	}
	return true;
}
bool 
Matrice200::djiMoveZToTarget(float target_alt_m){
	if(_flightStatus != VehicleStatus::FlightStatus::IN_AIR){
		DWAR(__FILE__,__LINE__,"Running move to z target, but the vehicle is not in air.",SocketPrintFd);	
		return false;
	}
	return djiMoveZByOffset(target_alt_m-_height_fusioned);
}
/*
*	Turn Vehicle head to target value
*	param target_head_deg: target head. uint: deg
*	param yaw_threshold_in_deg: threshold value. default:1 deg
*/
bool 
Matrice200::djiTurnHead(float target_head_deg,float yaw_threshold_in_deg){
	if(_flightStatus != VehicleStatus::FlightStatus::IN_AIR){
		DWAR(__FILE__,__LINE__,"Run turn head,but  the vehicle is not in air.",SocketPrintFd);	
		return false;
	}
	
	float xcmd=0,ycmd=0;
	float zcmd=_height_fusioned;
	// FLY MODE flags <note>: this need z control or alt maybe change ,because turn head 
	uint8_t ctrl_flag = Control::YAW_ANGLE | Control::VERTICAL_POSITION;
	while(true){
		Control::CtrlData data(ctrl_flag,xcmd,ycmd,zcmd,target_head_deg);
		_vehicle->control->flightCtrl(data);
		usleep(20000);  //20ms
		float current_yaw_in_rad=toEulerAngle(static_cast<void*>(&_quaternion)).z;	
		//check if finished turn head
		if(std::fabs(current_yaw_in_rad*RAD2DEG-target_head_deg) < yaw_threshold_in_deg){
			break;	
		}
		//run hook function
		LuaParser::runLuaHookFunction();
	}
	return true;
}
/*
* hover the dji vehicle to current pos
*/
bool
Matrice200::djiHover(){
	if(_flightStatus == VehicleStatus::FlightStatus::IN_AIR){
		//vehicle in air set pos offset 0,0,0 and velocity 0,0,0 and head to current head
		float current_yaw_in_rad=toEulerAngle(static_cast<void*>(&_quaternion)).z;
		djiMoveByPosOffset(0,0,0,current_yaw_in_rad*RAD2DEG);
		djiMoveByVelocity(0,0,0);
	}
	return true;
}
/* 
*	move vehicle by the x and y offset in same alt frame:NEU
*	parm target_x_m,target_y_m: horizontal offset 
*	param pos_threshold_in_m: threshold for stop point
*/
bool 
Matrice200::djiMoveX_YByOffset(float target_x_m, float target_y_m, float pos_threshold_in_m){
	if(_flightStatus != VehicleStatus::FlightStatus::IN_AIR){
		DWAR(__FILE__,__LINE__,"Runing move by x_y offset,but vehicle is not in air.",SocketPrintFd);	
		return false;
	}
	//record the start point
	TypeMap<TOPIC_GPS_FUSED>::type task_startPoint=_current_lat_lon;
	TypeMap<TOPIC_GPS_FUSED>::type current_lat_lon;
	
	Telemetry::Vector3f localoffset;
	float x_speed_factor=0;
	float y_speed_factor=0;
	// FLY MODE flags
	uint8_t ctrl_flag = Control::HORIZONTAL_POSITION | Control::HORIZONTAL_GROUND | Control::VERTICAL_POSITION |Control::YAW_ANGLE | Control::STABLE_ENABLE;		
	while(true){
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
		//run hook function
		LuaParser::runLuaHookFunction();
		
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
	}
		
	// do a emergencyBrake 
	_vehicle->control->emergencyBrake();
	return true;
}
bool 
Matrice200::djiMoveByPosOffset(float x_offset_Desired,float y_offset_Desired ,float z_offset_Desired, float yaw_Desired,float pos_threshold_in_m,float yaw_threshold_in_deg){
	if(_flightStatus != VehicleStatus::FlightStatus::IN_AIR){
		DWAR(__FILE__,__LINE__,"Run the move by pos offset,but vehicle is not in air",SocketPrintFd);	
		return false;
	}
	Telemetry::Vector3f localoffset;
	//record the start point
	TypeMap<TOPIC_GPS_FUSED>::type 		task_startPoint		=_current_lat_lon;
	TypeMap<TOPIC_HEIGHT_FUSION>::type 	task_startAltitude	=_height_fusioned;
	float x_speed_factor=0;
	float y_speed_factor=0;
	float z_speed_factor=0;
	double yaw_threshold_in_rad	=	DEG2RAD * yaw_threshold_in_deg;
	double yaw_desired_rad      =   DEG2RAD * yaw_Desired;
	while(true){ 		
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
		//run hook function
		LuaParser::runLuaHookFunction();
			
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
	}
	// do a emergencyBrake 
	_vehicle->control->emergencyBrake();		
	return true;
}
