/*
* @file PayloadBase.h
* @author: JYL
* @email:jiyingliang369@126.com
* @date: 2019.06.12
*
*/
/*
*	PayloadBase is a base class for control the payload that mount on the dji UAV.
*	user need inherit PayloadBase and extend the more control for specific payload.
*	Payload support RC and lua script control,user need regist response function in init().
*/
#pragma once
#include <stdlib.h>
#include <string.h>
#include <map>
#include "dji_telemetry.hpp"
#define MAX_BUTTON 4
/*
*	rc button response callback
*/
class PayloadBase;

typedef void (*rcResponseCallback)(int rc_number,int16_t rc_value);
enum BUTTON{
	BUTTON_C1=0,
	BUTTON_C2=1,
	BUTTON_C3=2,
	BUTTON_C4=3
};
/*
*	struct for store the user regist RC function info.
*	allow regist name for RC buttuon, but the name size is <10
*/
typedef struct BUTTON_FUNCTION{
	enum BUTTON _number;
	char _name[10];
	int16_t _value;
	rcResponseCallback _func;
}button_response_t; 

/**/
typedef void* UserData;
/*
*	lua script response callback.
*/
typedef void (*luaScriptResponseCallback)(UserData userdate);

class PayloadBase{
public:
	explicit PayloadBase();
	PayloadBase(const PayloadBase&)=delete;
	PayloadBase& operator =(const PayloadBase&)=delete;
	virtual ~PayloadBase();
	virtual bool init()=0;
	/*
	*	regist payload control function by lua script call.
	*	param lua_call_function: the control function handler ,type is luaScriptResponseCallback. 
	*	param function_name: the function name.
	*	return: true or false 
	*/
	static bool registLuaCallFunction(luaScriptResponseCallback lua_call_function, const std::string& function_name);
	/*
	*	system will call this function if run lua script call regist function ,user shoul't call this function never.	
	*/
	static bool runLuaCallFunction(const std::string& function_name, UserData userdate);
	/*
	*	regist payload control function by RC call. 
	*	param button_number: RC button number [1,2,3,4]
	*	param response_function: callback function handler
	*	param button_name: button name ,user can regist a name for button, but size <10 default: button.
	*   return: true or false
	*/	
	static bool registRCResponseFunction(enum BUTTON button_number,rcResponseCallback response_function,const char* button_name="button");
	/*	
	*	system will call this function if RC button change , usr should't call this function never.
	*/
	static void runRcFunction(DJI::OSDK::Telemetry::TypeMap<DJI::OSDK::Telemetry::TOPIC_RC_FULL_RAW_DATA>::type* rc);
	/*
	*	get RC value by button number (1,2,3,4)
	*/
	int16_t getRCValueByNumber(enum BUTTON button_number);
	/*
	*	get RC value by button name
	*/
	int16_t getRCValueByName(const std::string& button_name);
private:
	/*
	*	struct for store user regsited rc response function info.
	*/	
	static button_response_t _button_response[MAX_BUTTON];
	/*
	*	map for store user registd lua control function info.
	*/
	static std::map<std::string,luaScriptResponseCallback> *_lua_response;
};
