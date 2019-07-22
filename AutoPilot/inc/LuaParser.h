#pragma once
#include <string>
#include <thread>
#include "lua.hpp"
#include "FlightCore.h"
#include "FlightStatus.h"

class FlightCore;
class LuaParser{
public:
	/*constructor*/
	LuaParser();
	/*destructor*/
	~LuaParser();
	/*parser init funcion*/	
	bool LuaParserInit();
	/*open lua script */
	bool LuaScriptOpenAndRun(const std::string &lua_file_name_path,bool need_new_thread=false);
	/*interrupt runing script */
	static void LuaInterruptRuning(const std::string& reason);
	/*pause the lua script run,untill command to go on */
	static void LuaRunPause(const std::string& reason);
	/*go on the lua script run*/
	static void LuaRunGoOn(const std::string& reason);
	/*get the run status of lua script thread*/
	static bool LuaScriptThreadRunning(){return _lua_script_thread_running;}
	/* get value from lua script statck*/
	bool LuaGettableValueByIndex(const char* table_name,int index,std::string& value);
	/*get value from lua script by name*/	
	bool LuaGettableValueByName(const char* table_name,const  char* value_name, std::string& value);
	
private:
	/*lua run thread*/
	void LuaParserRunThread();
	/*flight core*/	
	//FlightCore * _flight_core=nullptr;
	/*lua handle*/	
	static lua_State* _lua;
	/*run lua thread handle*/
	static std::thread* _lua_script_run_thread;
	/*lua script thread running flag*/
	static bool _lua_script_thread_running;
};

