/*
* @file LuaInterface.cpp
* @author: JYL
* @email:jiyingliang369@126.com
* @date: 2019.06.13
*
*/
#include <string>
#include <cmath>
#include <sys/time.h>
#include "LuaInterface.h"
#include "Message.h"
#include "PayloadBase.h"
#include "geo.h"

using namespace DJI::OSDK;
using namespace DJI::OSDK::Telemetry;

FlightCore* LuaInterface::_flight_core=nullptr;

LuaInterface::LuaInterface()
{
}

LuaInterface::~LuaInterface(){
}

void LuaInterface::LuaStop(lua_State* lua, lua_Debug* ar){
	lua_sethook(lua,NULL,0,0);
	FLIGHTLOG("Lua script running Interrupt");
	luaL_error(lua,"Run Interrupt...");
}

int LuaInterface::LuaTakeoff(lua_State* lua){
	// write message to log buff
	FLIGHTLOG("Run takeoff by lua script.");
	if(!_flight_core->djiTakeoff()){
		DWAR("Runing takeoff err.");
		lua_pushboolean(lua,LUA_FAIL);
		lua_error(lua); // interrupt run the script
		return LUA_FAIL;
	}
	lua_pushboolean(lua,LUA_SUCESS);
	return LUA_SUCESS;
}
int LuaInterface::Lualand(lua_State* lua){
	// write message to log buff
	FLIGHTLOG("Run Land by lua script.");
	if(!_flight_core->djiLanding()){
		DWAR("Runing land err. ");
		lua_pushboolean(lua,LUA_FAIL);
		lua_error(lua); // interrupt run the script
		return LUA_FAIL;
	}
	lua_pushboolean(lua,LUA_SUCESS);
	return LUA_SUCESS;
}
int LuaInterface::LuaGoHome(lua_State* lua){
	// write message to log buff
	FLIGHTLOG("Run go home.");
	if(!_flight_core->djiGoHome()){
		DWAR("Runing go home err. ");
		lua_pushboolean(lua,LUA_FAIL);
		lua_error(lua); // interrupt run the script
		return LUA_FAIL;
	}
	lua_pushboolean(lua,LUA_SUCESS);
	return LUA_SUCESS;
}
int LuaInterface::LuaFlyByGPS(lua_State* lua){
	if(lua_gettop(lua) != 2){
		DWAR("Param Need Latitude and longitude. ");
		luaL_error(lua,"Param Need Latitude and longitude."); // interrupt run the script
		return LUA_FAIL;
	}
	double target_lat=luaL_checknumber(lua,1);// get the target lat
	double target_lon=luaL_checknumber(lua,2);
	// write message to log buff
	FLIGHTLOG("Move to GPS point:" +std::to_string(target_lat) + "," + std::to_string(target_lon));
	//check the lat and long limit	
	if((target_lat> 90 || target_lat < -90) || (target_lon > 180 || target_lon < -180)){
		DWAR("Latitude and longitude is out the range.");
		luaL_error(lua,"Latitude and longitude is out the range."); // interrupt run the script
		return LUA_FAIL;
	}
	//do fly
	if(!_flight_core->djiMoveByGPS(target_lat,target_lon)){
		DWAR("Runing move by gps err.");
		lua_pushboolean(lua,LUA_FAIL);
		luaL_error(lua,"Runing move by gps err."); // interrupt run the script
		return LUA_FAIL;
	}
	lua_pushboolean(lua,LUA_SUCESS);
	return LUA_SUCESS;
}
int LuaInterface::LuaFlyByVelocity(lua_State* lua){
	if(lua_gettop(lua) != 3){
		DWAR("Need vx,vy,vz.");
		lua_error(lua); // interrupt run the script
		return LUA_FAIL;
	}
	double vx=luaL_checknumber(lua,1);// get the target vx
	double vy=luaL_checknumber(lua,2);// get the target vy
	double vz=luaL_checknumber(lua,3);// get the target vz
	if(!_flight_core->djiMoveByVelocity(vx,vy,vz)){
		DWAR("Runing move by velocity err.");
		lua_pushboolean(lua,LUA_FAIL);
		lua_error(lua); // interrupt run the script
		return LUA_FAIL;
	}
	lua_pushboolean(lua,LUA_SUCESS);
	return LUA_SUCESS;
}

int LuaInterface::LuaFlyByBearingAndDistance(lua_State* lua){
	if(lua_gettop(lua) != 2){
		DWAR("Need a bearing(deg)  and a distance(m) argument.");
		lua_error(lua); // interrupt run the script
		return LUA_FAIL;
	}
	float bearing=luaL_checknumber(lua,1);// get the target bearing uint:deg
	float distance=luaL_checknumber(lua,2);// get the target distance
	if(bearing<0 || bearing>360){
		DWAR("Target bearing is out the range.");
		lua_error(lua); // interrupt run the script
		return LUA_FAIL;
	}
	if(!_flight_core->djiMoveByBearingAndDistance(bearing,distance)){
		DWAR("Running move by bearing and distance err.");
		lua_error(lua); // interrupt run the script
		return LUA_FAIL;
	}
	lua_pushboolean(lua,LUA_SUCESS);
	return LUA_SUCESS;
}

int LuaInterface::LuaTurnHead(lua_State* lua){
	if(lua_gettop(lua) != 1){
		DWAR("Param need the target head(deg).");
		lua_error(lua); // interrupt run the script
		return LUA_FAIL;
	}
	float target_head=luaL_checknumber(lua,1);
	// write message to log buff
	FLIGHTLOG("Turn head :" + std::to_string(target_head)+"deg.");
	
	if(target_head < 0 || target_head > 360){
		DWAR("Target head is out the range");
		lua_error(lua); // interrupt run the script
		return LUA_FAIL;
	}
	if(!_flight_core->djiTurnHead(target_head)){
		DWAR("Runing turn head err. ");
		lua_pushboolean(lua,LUA_FAIL);
		lua_error(lua); // interrupt run the script
		return LUA_FAIL;
	}
	lua_pushboolean(lua,LUA_SUCESS);
	return LUA_SUCESS;
}

int LuaInterface::LuaClimbTo(lua_State* lua){
	if(lua_gettop(lua) != 1){
		DWAR("Param need the target alt(m)");
		lua_error(lua); // interrupt run the script
		return LUA_FAIL;
	}
	//get param from lua 
	float target_alt=luaL_checknumber(lua,1);
	// write message to log buff
	FLIGHTLOG("Climb To :" + std::to_string(target_alt)+"m.");
	if(!_flight_core->djiMoveZToTarget(target_alt)){
		DWAR("Runing climb err. ");
		lua_pushboolean(lua,LUA_FAIL);
		lua_error(lua); // interrupt run the script
		return LUA_FAIL;
	}
	
	lua_pushboolean(lua,LUA_SUCESS);
	return LUA_SUCESS;
}
int LuaInterface::LuaClimbBy(lua_State* lua){
	if(lua_gettop(lua) != 1){
		DWAR("Param need the target alt(m).");
		lua_error(lua); // interrupt run the script
		return LUA_FAIL;
	}
	//get param from lua 
	float target_alt=luaL_checknumber(lua,1);
	// write message to log buff
	FLIGHTLOG("Climb By :" + std::to_string(target_alt)+"m.");

	if(!_flight_core->djiMoveZByOffset(target_alt)){
		DWAR("Runing climb by err. ");
		lua_pushboolean(lua,LUA_FAIL);
		lua_error(lua); // interrupt run the script
		return LUA_FAIL;
	}
	
	lua_pushboolean(lua, LUA_SUCESS);
	return LUA_SUCESS;
}
int LuaInterface::LuaDelay(lua_State* lua){
	if(lua_gettop(lua) != 1){
		DWAR("LuaDelay need a time argument (ms). ");
		lua_error(lua); // interrupt run the script
		return LUA_FAIL;
	}
	float time_ms=luaL_checknumber(lua,1);
	FLIGHTLOG("Delay :" + std::to_string(time_ms)+"ms.");
	struct timeval tv;
	double start_time=0;
	double now_time;
	gettimeofday(&tv,NULL);
	start_time=tv.tv_sec*1000+tv.tv_usec*0.001;
	while(1){
		usleep(3000);//3ms
		gettimeofday(&tv,NULL);
		now_time=tv.tv_sec*1000+tv.tv_usec*0.001;
		if(now_time-start_time>time_ms){ 
			break;
		}
		usleep(3000);//3ms
	}
	return LUA_SUCESS;
}

int LuaInterface::LuaGetLocationGPS(lua_State* lua){
	TypeMap<TOPIC_GPS_FUSED>::type 		current_lat_lon;
	TypeMap<TOPIC_HEIGHT_FUSION>::type 	altitude_fusioned;
	_flight_core->getVehicleGPS(&current_lat_lon);
	_flight_core->getVehicleAltitude(&altitude_fusioned);
	double lat_deg=current_lat_lon.latitude*RAD2DEG; //deg
	double lon_deg=current_lat_lon.longitude*RAD2DEG; //deg
	float height=altitude_fusioned;//m
	
	//push to lua
	lua_pushnumber(lua,lat_deg);
	lua_pushnumber(lua,lon_deg);
	lua_pushnumber(lua,height);
	usleep(50000); //50ms
	return 3;
}
int LuaInterface::LuaGetVelocity(lua_State* lua){
	TypeMap<TOPIC_VELOCITY>::type velocity;
	_flight_core->getVehicleVelocity(&velocity);
	
	lua_pushnumber(lua,velocity.data.x);
	lua_pushnumber(lua,velocity.data.y);
	lua_pushnumber(lua,velocity.data.z);
	usleep(50000); //50ms
	return 3;
}
int LuaInterface::LuaGetHead(lua_State* lua){
	float head=_flight_core->getVehicleBearing();
	lua_pushnumber(lua,head);
	usleep(50000); //50ms
	return 1;
}

int LuaInterface::LuaGetAlt(lua_State* lua){
	TypeMap<TOPIC_HEIGHT_FUSION>::type 	altitude_fusioned;
	_flight_core->getVehicleAltitude(&altitude_fusioned);
	float height=altitude_fusioned;//m
	
	//push to lua
	lua_pushnumber(lua,height);
	usleep(50000); //50ms
	return 1;
}
/*
*	use to call user function that regist in payload class
*/
int LuaInterface::LuaCall(lua_State* lua){
	// get the param count of LuaCall
	int param_count=lua_gettop(lua);
	if(param_count<1){
		DWAR("LuaCall  need at least one param for function name");
		lua_error(lua); // interrupt run the script
		return LUA_FAIL;
	}
	//get the first param for function name
	std::string function_name;
	function_name=luaL_checkstring(lua,1);
	
	float* _func_param=nullptr;
	std::string param_log;
	if(param_count>=2){
		//new heap for param 
		_func_param=new float[param_count-1];
		for(int i=0;i < param_count-1; i++){
			_func_param[i]=luaL_checknumber(lua,i+2);
			param_log.append(std::to_string(_func_param[i])+",");
		}
	}
	// record function call
	if(param_log.size()>0) 
		param_log.pop_back();
	FLIGHTLOG("Call function: " + function_name +"(" + param_log +")");
	
	// call user regist function
	bool result=PayloadBase::runLuaCallFunction(function_name,(UserData)_func_param);
	if (_func_param != nullptr) 
		delete[] _func_param;
	
	return result? LUA_SUCESS : LUA_FAIL;
}

int LuaInterface::LuaShootPhoto(lua_State* lua){
	FLIGHTLOG("Shoot Photo");
	if(!_flight_core->djiShootPhoto()){
		DWAR("Shoot photo err.");
		lua_error(lua); // interrupt run the script
		return LUA_FAIL;
	}
	lua_pushboolean(lua, LUA_SUCESS);
	return LUA_SUCESS;
}
int LuaInterface::LuaVideoStart(lua_State* lua){
	FLIGHTLOG("Start video");
	if(!_flight_core->djiVideoStart()){
		DWAR("Start video err.");
		lua_error(lua); // interrupt run the script
		return LUA_FAIL;
	}
	lua_pushboolean(lua, LUA_SUCESS);
	return LUA_SUCESS;
}
int LuaInterface::LuaVideoStop(lua_State* lua){
	FLIGHTLOG("Stop video");
	if(!_flight_core->djiVideoStop()){
		DWAR("Stop video err.");
		lua_error(lua); // interrupt run the script
		return LUA_FAIL;
	}
	return LUA_SUCESS;
}
int LuaInterface::LuaCameraZoom(lua_State* lua){
	if(lua_gettop(lua) != 2){
		DWAR("Need mode and value 2 argument (deg). ");
		lua_error(lua); // interrupt run the script
		return LUA_FAIL;
	}
	int mode=luaL_checkinteger(lua,1); //get control mode
	int value=luaL_checkinteger(lua,2); //get control value
	if(!_flight_core->djiCameraZoom(mode,value)){
		DWAR("Camera zoom err. ");
		lua_error(lua); // interrupt run the script
		return LUA_FAIL;
	}
	return LUA_SUCESS;
}
int 
LuaInterface::LuaSetGimbalAngle(lua_State* lua){
	if(lua_gettop(lua) != 3){
		DWAR("Need roll,pitch,yaw 3 argument (deg). ");
		lua_error(lua); // interrupt run the script
		return LUA_FAIL;
	}
	float roll_deg=luaL_checknumber(lua,1);
	float pitch_deg=luaL_checknumber(lua,2);
	float yaw_deg=luaL_checknumber(lua,3);
	if(fabs(yaw_deg)>320){
		DWAR("desire Yaw angle is out of limit");
		yaw_deg=yaw_deg>0? 320:-320;
	}
	if( fabs(roll_deg)>35){
		DWAR("desire Roll angle is out of limit");
		yaw_deg=roll_deg>0? 35:-35;
	}
	if(pitch_deg<-90 || pitch_deg>30){
		DWAR("desire Pitch angle is out of limit");
		yaw_deg=pitch_deg>0? 30:-90;
	}
	if(!_flight_core->djiSetGimbalAngle(roll_deg,pitch_deg,yaw_deg)){
		DWAR("Set gimbal angle err.");
		lua_error(lua); // interrupt run the script
		return LUA_FAIL;	
	}
	return LUA_SUCESS;
}
int 
LuaInterface::LuaGetGimbalAngle(lua_State* lua){
	TypeMap<TOPIC_GIMBAL_ANGLES>::type	gimbal_angle;
	_flight_core->getGimbalAngle(&gimbal_angle);
	
	lua_pushnumber(lua,gimbal_angle.y);
	lua_pushnumber(lua,gimbal_angle.x);
	lua_pushnumber(lua,gimbal_angle.z);
	usleep(50000); //50ms
	return 3;
}
/*
*	Test motor
*/
int 
LuaInterface::LuaTestMotor(lua_State* lua){
	FLIGHTLOG("Start test motor.");
	if(!_flight_core->djiArmMotor()){
		DWAR("Start motor err.");
		lua_error(lua); // interrupt run the script
		return LUA_FAIL;
	}
	int count=0;
	while(count<5){
		count++;
		sleep(1); //wating 10*1=10s
	}
	if(!_flight_core->djiDisarmMotor()){
		DWAR("Stop motor err.");
		lua_error(lua); // interrupt run the script
		return LUA_FAIL;
	}
	FLIGHTLOG("Stop test motor.");
	return LUA_SUCESS;
}
