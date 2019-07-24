/*
* @file FlightParser.cpp
* @author: JYL
* @email:jiyingliang369@126.com
* @date: 2019.06.10
*
*/
#include <fstream>
#include <iostream>
#include "LuaParser.h"
#include "LuaInterface.h"
#include "Message.h"

std::thread* LuaParser::_lua_script_run_thread=nullptr;
lua_State* LuaParser::_lua=nullptr;
bool 	LuaParser::_lua_script_thread_running=false;

LuaParser::LuaParser(){
	if(!LuaParserInit()){
		DDBUG("Lua Parser creat err");
	}
}

LuaParser:: ~LuaParser(){
	if(_lua)
		lua_close(_lua);
	
	if(_lua_script_run_thread != nullptr){
		_lua_script_run_thread->join();
		delete _lua_script_run_thread;
		_lua_script_run_thread=nullptr;	
	}
}

/*parser init funcion*/	
bool LuaParser::LuaParserInit(){
	if(_lua)
		lua_close(_lua);

	//creat a new lua 
	_lua=luaL_newstate();
	if(_lua==nullptr){
		DERR("Creat lua state err");
		return false;
	}
	// open the lua stand lib	
	luaL_openlibs(_lua);
	
	//register the lua script function
	lua_register(_lua,"LuaTakeoff",&LuaInterface::LuaTakeoff);
	lua_register(_lua,"Lualand",&LuaInterface::Lualand);
	lua_register(_lua,"LuaFlyByGPS",&LuaInterface::LuaFlyByGPS);
	lua_register(_lua,"LuaFlyByVelocity",&LuaInterface::LuaFlyByVelocity);
	lua_register(_lua,"LuaFlyByBearingAndDistance",&LuaInterface::LuaFlyByBearingAndDistance);
	lua_register(_lua,"LuaTurnHead",&LuaInterface::LuaTurnHead);
	lua_register(_lua,"LuaClimbTo",&LuaInterface::LuaClimbTo);
	lua_register(_lua,"LuaClimbBy",&LuaInterface::LuaClimbBy);
	lua_register(_lua,"LuaGoHome",&LuaInterface::LuaGoHome);
	lua_register(_lua,"LuaDelay",&LuaInterface::LuaDelay);
	lua_register(_lua,"LuaGetLocationGPS",&LuaInterface::LuaGetLocationGPS);
	lua_register(_lua,"LuaGetHead",&LuaInterface::LuaGetHead);
	lua_register(_lua,"LuaGetAlt",&LuaInterface::LuaGetAlt);
	lua_register(_lua,"LuaCall",&LuaInterface::LuaCall);
	lua_register(_lua,"LuaShootPhoto",&LuaInterface::LuaShootPhoto);
	lua_register(_lua,"LuaVideoStart",&LuaInterface::LuaVideoStart);
	lua_register(_lua,"LuaVideoStop",&LuaInterface::LuaVideoStop);
	lua_register(_lua,"LuaCameraZoom",&LuaInterface::LuaCameraZoom);
	lua_register(_lua,"LuaSetGimbalAngle",&LuaInterface::LuaSetGimbalAngle);
	lua_register(_lua,"LuaGetGimbalAngle",&LuaInterface::LuaGetGimbalAngle);
	lua_register(_lua,"LuaTestMotor",&LuaInterface::LuaTestMotor);
	
	return true;
}
/*
*open the lua script and run in another thread
*param: lua file name path
*/
bool LuaParser::LuaScriptOpenAndRun(const std::string &lua_file_name_path,bool need_new_thread){
	//check 1: lua file path valid
	if(lua_file_name_path.empty()){
		DWAR("Lua script path is empty.");
		return false;
	}
	// check 2: if there is a thread running  
	if(_lua_script_thread_running){
		DWAR("A lua script thread is running,please waiting or break it.");
		return false;
	}
	//check 3: the lua parser healthy
	int err_code=lua_status(_lua);		
	if( err_code != LUA_OK){
		DWAR("Lua status is not OK,err code: "+std::to_string(err_code));
		return false;
	}
	// clear the all interrupt hook before run the lua script
	lua_sethook(_lua,NULL,0,0);
	//load lua script frome file
	err_code=luaL_loadfile(_lua,lua_file_name_path.c_str());
	if ( err_code != LUA_OK){
		DWAR("Loading lua script file err,err code: "+ std::to_string(err_code));
		return false;
	}
	if(need_new_thread){
		//creat a new thread for run the lua script	
		if(_lua_script_run_thread != nullptr){
			_lua_script_run_thread->join();
			delete _lua_script_run_thread;
			_lua_script_run_thread=nullptr;
		}
#ifdef OFFLINE_DEBUG
#else		
		// clear the while loop break flag in flight core class before run the lua script
		FlightCore::djiNeedBreakAutoControl(false);
		// get flight core ctr authority
		if(!FlightCore::djiGetControlAuthority()){
			DWAR("Lua thread get ctr authority err.");	
			return false;
		}
#endif
		FLIGHTLOG("Creat a New thread for lua script run...");
		_lua_script_thread_running=true;
		_lua_script_run_thread = new std::thread(&LuaParser::LuaParserRunThread,this);
		
	}else{
		LuaParserRunThread();
	}
	return true;
}
void 
LuaParser::LuaParserRunThread(){
	FLIGHTLOG("The Lua script start Running... ");
	int err_code=lua_pcall(_lua,0,0,0);
	if(err_code != LUA_OK){
		DWAR("Run lua script err,err code: "+ std::to_string(err_code));
	}else{
		FLIGHTLOG("The lua script run Complete.");
	}
	//clear the thread runng flag
	if(_lua_script_thread_running){
		_lua_script_thread_running=false;
#ifdef OFFLINE_DEBUG
#else
		if(!FlightCore::djiReleaseControlAuthority()){
			DWAR("Lua thread release ctr authority err.");
		}
#endif
	}
}

void 
LuaParser::LuaInterruptRuning(const std::string& reason){
	if(_lua_script_thread_running){
		DWAR("Break lua script : "+reason);
		// set lua debug hook for stop
		lua_sethook(_lua,&LuaInterface::LuaStop,LUA_MASKCALL | LUA_MASKRET | LUA_MASKCOUNT,1);
		// break while loop in the flight core class 
		FlightCore::djiNeedBreakAutoControl(true);
		//if in pause mode then,force exit the pause 
		LuaInterface::_need_go_on_run=true;
	}
}
void 
LuaParser::LuaRunPause(const std::string& reason){
	if(_lua_script_thread_running){
		//first: set the exit flag to false
		LuaInterface::_need_go_on_run=false;
		FLIGHTLOG("Lua script running pause by "+reason);
		//second: set lua debug hook for into a while loop
		lua_sethook(_lua,&LuaInterface::LuaPause,LUA_MASKCALL | LUA_MASKRET | LUA_MASKCOUNT,1);
	}
}
void 
LuaParser::LuaRunGoOn(const std::string& reason){
	if(_lua_script_thread_running){
		//set pause exit flag to true 
		FLIGHTLOG("Lua script running Go On by "+reason);
		LuaInterface::_need_go_on_run=true;
	}
}
bool
LuaParser::LuaGettableValueByIndex(const char* table_name,int index,std::string& value){
	if(_lua==nullptr){
		DERR(" Lua state is nullptr!");
		return false;
	}		
	lua_getglobal(_lua,table_name);
	if(!lua_istable(_lua,-1)){
		DERR("Get Lua table err!");
		return false;	
	}else{
		lua_pushinteger(_lua, index);
		lua_gettable(_lua,1);
		value.append(lua_tostring(_lua,-1));
	}
	lua_pop(_lua,-1);
	return true;
}
bool
LuaParser::LuaGettableValueByName(const char* table_name, const char* value_name, std::string& value){
	if(_lua==nullptr){
		DERR("Lua state is nullptr!");
		return false;
	}		
	lua_getglobal(_lua,table_name);
	if(!lua_istable(_lua,-1)){
		DERR("Get Lua table err!");
		return false;	
	}else{
		lua_pushstring(_lua, value_name);
		lua_gettable(_lua,1);
		value.append(lua_tostring(_lua,-1));
	}
	lua_pop(_lua,-1);
	return true;
}

