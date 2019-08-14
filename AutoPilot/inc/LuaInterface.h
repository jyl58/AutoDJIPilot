/*
* @file LuaInterface.h
* @author: JYL
* @email:jiyingliang369@126.com
* @date: 2019.06.10
*
*/
#pragma once
#include <iostream>
#include "FlightStatus.h"
#include "FlightCore.h"
#include "lua.hpp"

#define LUA_SUCESS 1
#define LUA_FAIL 0

typedef int (*reg_function)(lua_State*);

class FlightCore;
class LuaInterface
{
public:
	
	/*constructor*/	
	LuaInterface();
	/*Do not allow copy and =*/
	LuaInterface(const LuaInterface&)=delete;
	LuaInterface& operator=(const LuaInterface&)=delete;
	/*destructor*/
	~LuaInterface();
		
	////////////SYS CONTROL////////////
	static void LuaStop(lua_State* lua,lua_Debug* ar);
	
	/**********************************
	* pause the lua script run into a while loop untill exit by command
	************************************/
	static void LuaPause(lua_State* lua, lua_Debug* ar);
	
	
	////////////flight control ////////
	/********************************
	* takeoff function for lua script
	* 
	********************************/
	static int LuaTakeoff(lua_State* lua);

	/*******************************
	* land for lua script
	* ******************************/
	static int Lualand(lua_State* lua);
	
	/*******************************
	*Go home the vehicle
	*******************************/
	static int LuaGoHome(lua_State* lua);

	/********************************
	* pos fly function for lua script
	* param: target lat and lon
	* unit: deg
	********************************/
	static int LuaFlyByGPS(lua_State* lua);

	/*********************************
	* velicity fly function for lua 
	* param vx,vy,vz.unit: m/s
	*********************************/
	static int LuaFlyByVelocity(lua_State* lua);
	/********************************
	* pos fly function for lua script
	* param: target distance and bearing
	* unit: m,deg(-180,180) 
	********************************/
	static int LuaFlyByBearingAndDistance(lua_State* lua);

	/********************************
	* turn head function for lua script
	* param: target head
	* unit: deg (-180,180)
	********************************/
	static int LuaTurnHead(lua_State* lua);

	/********************************
	* climb vehicle to point alt
	* param: target  alt
	* unit: m
	********************************/
	static int LuaClimbTo(lua_State* lua);

	/********************************
	* climb vehicle by point alt
	* param: target  alt
	* unit: m
	********************************/
	static int LuaClimbBy(lua_State* lua);

	/********************************
	* delay function for lua script
	* param: waiting time
	* unit: ms  
	********************************/
	static int LuaDelay(lua_State* lua);

	/********************************
	* get location function for lua script
	* return:  lat,lon,height
	* unit: deg ,m
	********************************/
	static int LuaGetLocationGPS(lua_State* lua);
	/********************************
	* return the vehicle velocity
	* return: vx,vy,vz.unit m/s
	********************************/
	static int LuaGetVelocity(lua_State* lua);
	/********************************
	* get head function for lua script
	* return:  head  
	* unit: deg
	********************************/
	static int LuaGetHead(lua_State* lua);

	/********************************
	* get fly alt function for lua script
	* return:  head  
	* unit: deg
	********************************/
	static int LuaGetAlt(lua_State* lua);
	

	////////////AUX control////////////
	/********************************
	*	lua call function in payload plugin.
	*	use: LuaCall("function_name",argument)
	********************************/
	static int LuaCall(lua_State* lua);
	/**********************************
	*	Test motor
	**********************************/
	static int LuaTestMotor(lua_State* lua);
	
	/*********************************
	*	camera control:shoot photo
	*	
	**********************************/
	static int LuaShootPhoto(lua_State* lua);
	/*********************************
	*	camera control:video start
	*	
	**********************************/
	static int LuaVideoStart(lua_State* lua);
	/*********************************
	*	camera control:video stop
	*	
	**********************************/
	static int LuaVideoStop(lua_State* lua);
	/********************************
	*	camera control: zoom
	*	param mode: zoom mode, 0:step;1:pos;2;speed
	*	param value: control value,step;pos[100,3000];speed[-100,100]
	********************************/
	static int LuaCameraZoom(lua_State* lua);
	/******************************
	*	set gimbal control
	*
	******************************/
	static int LuaSetGimbalAngle(lua_State* lua);

	/******************************
	*	get gimbal angle
	*
	******************************/
	static int LuaGetGimbalAngle(lua_State* lua);
	/*******************************
	* flight core
	*******************************/	
	static std::shared_ptr<FlightCore> _flight_core;
	
	static bool _need_go_on_run;
};
