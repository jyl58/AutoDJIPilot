#include <stdlib.h>
#include "DJI_MAVlinkBrige.h"
#include "MavlinkRouter.h"
#include "FlightCore.h"
#include "Message.h"
#include "geo.h"
class MavlinkRouter;
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
/* RC input callback for pyload use*/
void RCCallback(DJI::OSDK::Vehicle* vehicle,RecvContainer recvFrame,UserData usrData){
	TypeMap<TOPIC_RC_FULL_RAW_DATA>::type 	rc_full_raw_data;
	rc_full_raw_data	=vehicle->subscribe->getValue<TOPIC_RC_FULL_RAW_DATA>();
	//FLIGHTLOG("RC: BT1=" +std::to_string(rc_full_raw_data.lb2.rcC1)+", BT2="+std::to_string(rc_full_raw_data.lb2.rcC2)+", Bt3="+std::to_string(rc_full_raw_data.lb2.rcD1)+", BT4="+std::to_string(rc_full_raw_data.lb2.rcD2));
	/*TypeMap<TOPIC_RC>::type 	rc_data;
	rc_data	=vehicle->subscribe->getValue<TOPIC_RC>();
	std::cout<<"RC: mode="<<rc_data.mode<<std::endl;
	std::cout<<"RC: gear="<<rc_data.gear<<std::endl;
	std::cout<<"RC: pitch="<<rc_data.pitch<<std::endl;*/

}
