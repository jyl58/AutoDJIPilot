/*
* @file PayloadBase.h
* @author: JYL
* @email:jiyingliang369@126.com
* @date: 2019.06.13
*
*/
#include <stdio.h>
#include <iostream>
#include "PayloadBase.h"
#include "Message.h"
// init the button response struct
button_response_t PayloadBase::_button_response[]={
	{BUTTON_C1,"BUTTON1",0,nullptr},
	{BUTTON_C2,"BUTTON2",0,nullptr},
	{BUTTON_C3,"BUTTON3",0,nullptr},
	{BUTTON_C4,"BUTTON4",0,nullptr}
};
std::map<std::string,luaScriptResponseCallback>* PayloadBase::_lua_response=nullptr;

PayloadBase::PayloadBase(){
	//reset the button struct callback function
	for(int i=0; i< MAX_BUTTON; i++){
		_button_response[i]._func=nullptr;
	}	
	// new a map memery from heap
	if(_lua_response != nullptr){
		delete _lua_response;
		_lua_response=nullptr;
	}
	_lua_response=new std::map<std::string,luaScriptResponseCallback>();
}
PayloadBase::~PayloadBase(){
	if(_lua_response != nullptr){
		delete _lua_response;
		_lua_response=nullptr;
	}
}
bool 
PayloadBase::registRCResponseFunction(enum BUTTON button_number,rcResponseCallback response_function,const char* button_name){
	if (button_number >= MAX_BUTTON){
		DWAR("regist button number need <"+std::to_string(MAX_BUTTON));
		return false;
	}
	if(sizeof(button_name)>10){
		DWAR("regist button name legth  need < 10 ");
		return false;	
	}
	if(_button_response[button_number]._func != nullptr){
		DWAR("button"+std::to_string(button_number)+"already regist callback function");
	}
	_button_response[button_number]._number	=button_number;
	memcpy(_button_response[button_number]._name, button_name,10);
	_button_response[button_number]._func	=response_function;
	return true;
}
void 
PayloadBase::runRcFunction(DJI::OSDK::Telemetry::TypeMap<DJI::OSDK::Telemetry::TOPIC_RC_FULL_RAW_DATA>::type* rc){
	int16_t temp_rc[4];
	temp_rc[0]=	rc->lb2.rcC1;
	temp_rc[1]=	rc->lb2.rcC2;
	temp_rc[2]=	rc->lb2.rcD1;
	temp_rc[3]=	rc->lb2.rcD2;
	for (int i=0; i<MAX_BUTTON; i++){
		if(_button_response[i]._value==0){
			_button_response[i]._value = temp_rc[i];
		}else if(_button_response[i]._value != temp_rc[i]){
			//update button value
			_button_response[i]._value = temp_rc[i];
			//run the response function
			if( _button_response[i]._func != nullptr )_button_response[i]._func(i,_button_response[i]._value);		
		}
	}
}
int16_t 
PayloadBase::getRCValueByNumber(enum BUTTON button_number){
	return _button_response[button_number]._value;
}
int16_t 
PayloadBase::getRCValueByName(const std::string& button_name){
	for(int i=0;i<MAX_BUTTON; i++){	
		if(	button_name.compare(_button_response[i]._name) == 0){
			return _button_response[i]._value;
		}
	}
	return 0;
}


/*
*	regist lua call function
*/
bool 
PayloadBase::registLuaCallFunction(luaScriptResponseCallback lua_call_function, const std::string& function_name){
	if(!_lua_response->empty()){
		if(_lua_response->find(function_name) != _lua_response->end()){
			DWAR("function: "+function_name+" already regist callback function");
			return false;
		}
	}
	_lua_response->insert(std::map<std::string,luaScriptResponseCallback>::value_type(function_name,lua_call_function));
	return true;
}
bool 
PayloadBase::runLuaCallFunction(const std::string& function_name, UserData userdate){
	auto fun =_lua_response->find(function_name);
	if(fun != _lua_response->end()){
		//if find, then run user registed function		
		fun->second(userdate);
	}else{
		DWAR("Can't find the function: "+function_name);
		return false;
	}
	return true;
}
