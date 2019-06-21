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
	bool LuaScriptOpenAndRun(const std::string &lua_file_name,bool need_new_thread=false);
	/*interrupt runing script */
	void LuaInterruptRuning();
	/* get value from lua script statck*/
	bool LuaGettableValueByIndex(const char* table_name,int index,std::string& value);
	/*get value from lua script by name*/	
	bool LuaGettableValueByName(const char* table_name,const  char* value_name, std::string& value);
	
private:
	/*lua run thread*/
	void LuaParserRunThread(const std::string &lua_file_name_path);
	/*flight core*/	
	//FlightCore * _flight_core=nullptr;
	/*lua handle*/	
	lua_State* _lua=nullptr;
	/*run lua thread handle*/
	std::thread* _lua_script_run_thread=nullptr;
};

