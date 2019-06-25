#include <stdlib.h>
#include "DJI_MAVlinkBrige.h"
#include "MavlinkRouter.h"
#include "LuaParser.h"
#include "FlightCore.h"
#include "Message.h"
#include "geo.h"
class MavlinkRouter;
static int rc_break_auto_control_count=0; // static variable for this file
/*send global message by mavlink to GCS */
void GlobalPosCallback(DJI::OSDK::Vehicle* vehicle,RecvContainer recvFrame,UserData usrData){
	//get vehicle gps and alt
	TypeMap<TOPIC_GPS_FUSED>::type 	current_lat_lon;
	current_lat_lon			=	vehicle->subscribe->getValue<TOPIC_GPS_FUSED>();
	TypeMap<TOPIC_HEIGHT_FUSION>::type 	height_fusioned;
	height_fusioned			= vehicle->subscribe->getValue<TOPIC_HEIGHT_FUSION>();
	TypeMap<TOPIC_VELOCITY>::type 	gps_velocity;
	gps_velocity			=vehicle->subscribe->getValue<TOPIC_VELOCITY>();
	//send message by mavlink protocol
	//MavlinkRouter::sendLocation(current_lat_lon.latitude*RAD2DEG,current_lat_lon.longitude*RAD2DEG,altitude_fusioned,gps_velocity.x,gps_velocity.y,gps_velocity.z);
	FLIGHTLOG("Location: "+std::to_string(current_lat_lon.latitude*RAD2DEG)+","+std::to_string(current_lat_lon.longitude*RAD2DEG)+","+std::to_string(height_fusioned));
}
/* RC input callback for pyload use and break the auto fly control*/
void RCCallback(DJI::OSDK::Vehicle* vehicle,RecvContainer recvFrame,UserData usrData){
	
	//check if lua script thread is runing,if not return
	if(!LuaParser::LuaScriptThreadRunning()){
		rc_break_auto_control_count=0;
		return;
	}
	TypeMap<TOPIC_RC_WITH_FLAG_DATA>::type 	rc_witch_flag;
	rc_witch_flag	=vehicle->subscribe->getValue<TOPIC_RC_WITH_FLAG_DATA>();
	if(rc_witch_flag.roll != 0.0f || rc_witch_flag.pitch != 0.0f || rc_witch_flag.yaw != 0.0f || rc_witch_flag.throttle != 0.0f){
		rc_break_auto_control_count++;
	}else{
		rc_break_auto_control_count=0;
	}
	// 20 hz: 50ms*40=2000ms
	if(rc_break_auto_control_count >= 30){
		rc_break_auto_control_count=0;
		LuaParser::LuaInterruptRuning("RC operation.");
	}
}
