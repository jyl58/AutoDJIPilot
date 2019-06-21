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
	lua_register(_lua,"LuaSetGimbalAngle",&LuaInterface::LuaSetGimbalAngle);
	lua_register(_lua,"LuaGetGimbalAngle",&LuaInterface::LuaGetGimbalAngle);
	lua_register(_lua,"LuaTestMotor",&LuaInterface::LuaTestMotor);

	return true;
}
/*
*open the lua script and run in another thread
*param: lua file name path
*/
bool LuaParser::LuaScriptOpenAndRun(const std::string &lua_file_name,bool need_new_thread){
	//check lua file 	
	if(lua_file_name.empty()){
		DWAR("Lua script path is empty");
		return false;
	}

	std::ifstream lua_file(lua_file_name);
	if(!lua_file.is_open()){
		DWAR("Can't open the Lua script file");
		return false;
	}
	// close the file 
	lua_file.close();
	
	if(need_new_thread){
		//creat a new thread for run the lua script	
		if(_lua_script_run_thread != nullptr){
			_lua_script_run_thread->join();
			delete _lua_script_run_thread;
			_lua_script_run_thread=nullptr;
		}
	  	DDBUG("Creat a New thread for lua script run...");
		FLIGHTLOG("Creat a New thread for lua script run...");
		_lua_script_run_thread = new std::thread(&LuaParser::LuaParserRunThread,this,lua_file_name);
	}else{
		LuaParserRunThread(lua_file_name);
	}
	return true;
}
void 
LuaParser::LuaParserRunThread(const std::string &lua_file_name_path){
	std::ifstream lua_file(lua_file_name_path);	
	// go to the file end 
	lua_file.seekg(0,std::ios::end); 
	// report the file lenght
	int length=lua_file.tellg(); 
	//back the file begin
	lua_file.seekg(0,std::ios::beg); 	
	// requst the memery
	char* lua_script=new char[length+1]; 
	//read the file context 
	lua_file.read(lua_script,length);
	// close the file 
	lua_file.close();
	//load lua script 
	if (luaL_loadstring(_lua,lua_script) != LUA_OK){
		DWAR("Loading lua script context err.");			
		return;
	}
	FLIGHTLOG("The Lua script start Running... ");
	if(lua_pcall(_lua,0,0,0) != LUA_OK){
		DWAR("Run lua script err.");
		return ;	
	}
	FLIGHTLOG("The Lua Script run Complete.");
	/*delete the new memery for lua script */
	delete[] lua_script;
	std::cout<<"DJI_AUTO>";
}

void 
LuaParser::LuaInterruptRuning(){
	if(_lua_script_run_thread != nullptr)
		lua_sethook(_lua,&LuaInterface::LuaStop,LUA_MASKCALL | LUA_MASKRET | LUA_MASKCOUNT,1);
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
		DERR(" Lua state is nullptr!");
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

