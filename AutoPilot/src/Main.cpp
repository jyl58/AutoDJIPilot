/*
* @file Main.cpp
* @author: JYL
* @email:jiyingliang369@126.com
* @date: 2019.06.10
*
*/
#include <stdlib.h>
#include <vector>
#include <fstream>
#include <dlfcn.h>
#include "linux/LinuxHelpers.hpp"
#include "LuaParser.h"
#include "FlightCore.h"
#include "MavlinkRouter.h"
#include "FlightLog.h"
#include "LuaInterface.h"
#include "Message.h"
#include "PayloadBase.h"
#include "geo.h"

LinuxSetup* _linux_setup=nullptr;
FlightCore* _flight_core=nullptr;
LuaParser* _lua_parser=nullptr;
PayloadBase* _payload_base=nullptr;
bool main_thread_need_exit=false;
//cmd and param vector
std::vector<std::string>_cmd_and_param;
//dynamic lib 
void* dynamic_lib_handler=nullptr;

void autopilot_system_init(std::string& config_file_path){
	//first  init log sub thread for record sys message
	if(!FlightLog::FlightLogInit()){
		DERR("Flight log init err!");
		exit(1);
	}
	// creat a lua parser.
	_lua_parser		=new LuaParser();
	if(_lua_parser == nullptr)
		exit(1);
	//creat a liux enviroment init instance.
	_linux_setup	=new LinuxSetup(_lua_parser,config_file_path);
	if(_linux_setup == nullptr)
		exit(1);

	_flight_core	=new FlightCore(_linux_setup->getVehicle());
	if(_flight_core == nullptr ){
		DERR("Creat flight core err!");
		exit(1);
	}

	if(!_flight_core->flightCoreInit()){
		DERR("Flight core init err!");
		exit(1);
	}
	LuaInterface::_flight_core =_flight_core;
	MavlinkRouter::_flight_core=_flight_core;
	// int flight mavlink router thread	
	/*if (!MavlinkRouter::MavlinkRouterInit(_linux_setup->getMAVlinkDevPort()->c_str())){
		exit(1);	
	}*/
}
void autopilot_system_exit(){
	// the order of decontructor is important
	if (_lua_parser != nullptr ){
		delete _lua_parser;
		_lua_parser=nullptr;
	}
	if(_flight_core != nullptr){
		delete _flight_core;
		_flight_core= nullptr;
	}
	if(_linux_setup != nullptr) {
		delete _linux_setup;
		_linux_setup= nullptr;
	}
	MavlinkRouter::stopMAVlinkThread();
	FlightLog::FlightLogStop();
	//close the dynamic lib
	if(dynamic_lib_handler != nullptr)
		dlclose(dynamic_lib_handler);
}
void PrintFlightStatusCMD(){
	if(_flight_core==nullptr){
		std::cout<<"Flight Core is't run!"<<std::endl;
		return;
	}
	TypeMap<TOPIC_STATUS_FLIGHT>::type 		flightStatus;
	TypeMap<TOPIC_BATTERY_INFO>::type 		battery_info;
	TypeMap<TOPIC_STATUS_DISPLAYMODE>::type	display_mode;
	TypeMap<TOPIC_GPS_FUSED>::type 			current_lat_lon;
	TypeMap<TOPIC_GPS_SIGNAL_LEVEL>::type 	gps_signal_level;
	TypeMap<TOPIC_HEIGHT_FUSION>::type 		altitude_fusioned;
	TypeMap<TOPIC_GPS_DETAILS>::type 		gps_details;
	TypeMap<TOPIC_VELOCITY>::type 			velocity;
	TypeMap<TOPIC_GIMBAL_STATUS>::type		gimbal_status;
	TypeMap<TOPIC_GIMBAL_ANGLES>::type		gimbal_angle;
	_flight_core->getFlightStatus(&flightStatus);
	_flight_core->getFlightBatteryInfo(&battery_info);
	_flight_core->getVehicleGPS(&current_lat_lon);
	_flight_core->getVehicleGpsDetails(&gps_details);
	_flight_core->getVehicleVelocity(&velocity);
	_flight_core->getVehicleAltitude(&altitude_fusioned);
	_flight_core->getVehicleDisplay(&display_mode);	
	_flight_core->getVehicleGpsSignalLevel(&gps_signal_level);
	_flight_core->getGimbalStatus(&gimbal_status);
	float vehicle_head=_flight_core->getVehicleBearing();
	if(gimbal_status.mountStatus){
		_flight_core->getGimbalAngle(&gimbal_angle);
	}
	
	std::cout<<"**********Flight Status**************"<<std::endl;
	std::cout<<"Flight Mode:    "<<(int)display_mode<<std::endl;
	std::cout<<"Status:         "<<(int)flightStatus<<std::endl;
	std::cout<<"Battery Info:   "<<(int)battery_info.voltage*0.001<<"(v), "<<(int)battery_info.current*0.001<<"(A), "<<(int)battery_info.percentage<<"%"<<std::endl;
	std::cout<<"Positon(LLA):   "<<current_lat_lon.latitude*RAD2DEG<<"(deg),"<<current_lat_lon.longitude*RAD2DEG<<"(deg),"<<altitude_fusioned<<"(m)"<<std::endl;
	std::cout<<"Bearing:        "<<vehicle_head<<"(deg)"<<std::endl;
	std::cout<<"GPS Message:    "<<gps_details.hdop<<"(hdop),"<<gps_details.hacc<<"(hps),"<<gps_details.fix<<"(fix),"<<(int)gps_details.usedGPS<<"(number),"<<(int)gps_signal_level<<"(sig(0~5))"<<std::endl;
	std::cout<<"Velocity(m/s):  "<<velocity.data.x<<", "<<velocity.data.y<<", "<<velocity.data.z<<std::endl;
	if(gimbal_status.mountStatus){
	uint8_t gimbal_mode=_flight_core->getGimbalMode();
	std::cout<<"Gimbal Mode:	"<<(int)gimbal_mode<<std::endl;
	std::cout<<"Gimbal angle(rpy):"<<gimbal_angle.y<<"(deg),"<<gimbal_angle.x<<"(deg),"<<gimbal_angle.z<<"(deg)"<<std::endl;
	}
	std::cout<<"************************************"<<std::endl;
}
void PrintConfigMsgCMD(void){
	if(_linux_setup==nullptr){
		std::cout<<"config file parse err!"<<std::endl;	
		return ;
	}
	std::cout<<"**********Config Msg****************"<<std::endl;
	std::cout<<"DJI app id:           	    "<<_linux_setup->getDJIAppId()<<std::endl;
	std::cout<<"DJI app key:          	    "<<_linux_setup->getDJIAppKey()<<std::endl;
	std::cout<<"DJI device port:      	    "<<_linux_setup->getDJIDevicePort()<<std::endl;
	std::cout<<"DJI device baudrate:  	    "<<_linux_setup->getDJIDevicePortBaudrate()<<std::endl;
	std::cout<<"MAV device port:            "<<_linux_setup->getMAVlinkDevPort()<<std::endl;
	std::cout<<"MAV device port baudrate:   "<<_linux_setup->getMAVlinkDevPortBaudrate()<<std::endl;
	std::cout<<"************************************"<<std::endl;
}
void StopFlightLogCMD(void){

}
void StartFlightLogCMD(void){

}
void ShootPhoto(){
	if(_flight_core==nullptr){
		std::cout<<"Flight Core is't run!"<<std::endl;
		return;
	}	
	_flight_core->djiShootPhoto();
}
void RunVideo(){
	if(_flight_core==nullptr){
		std::cout<<"Flight Core is't run!"<<std::endl;
		return;
	}
	if(_cmd_and_param[1].empty()){
		std::cout<<"video CMD need a argument (start|stop)"<<std::endl;
		return;
	}
	if(_cmd_and_param[1].compare("start")==0){
		_flight_core->djiVideoStart();
		std::cout<<"video start..."<<std::endl;
	}else if (_cmd_and_param[1].compare("stop")==0){
		_flight_core->djiVideoStop();
		std::cout<<"video stop"<<std::endl;
	}else{
		std::cout<<"Don't support video's subcmd: "<<_cmd_and_param[1]<<std::endl;
	}
}
void SetGimbal(){
	if(_flight_core==nullptr){
		std::cout<<"Flight Core is't run!"<<std::endl;
		return;
	}
	TypeMap<TOPIC_GIMBAL_STATUS>::type		gimbal_status;
	_flight_core->getGimbalStatus(&gimbal_status);
	if(gimbal_status.mountStatus==0){
		std::cout<<"Gimbal is not mount!"<<std::endl;
		return;
	}
	if(_cmd_and_param[1].empty()){
		std::cout<<"gimbal CMD need a argument (angle|speed)"<<std::endl;
		return;
	}

	if(_cmd_and_param[1].compare("angle") == 0){
		float roll_deg=_cmd_and_param[2].empty()?  0 : std::stof(_cmd_and_param[2]);
		float pitch_deg=_cmd_and_param[3].empty()? 0 : std::stof(_cmd_and_param[3]);
		float yaw_deg=_cmd_and_param[4].empty()?   0 : std::stof(_cmd_and_param[4]);
		_flight_core->djiSetGimbalAngle(roll_deg,pitch_deg,yaw_deg);
	}else if(_cmd_and_param[1].compare("speed") == 0){
		//TODO: add speed 	
	}else{
		std::cout<<"Don't support gimbal's subcmd: "<<_cmd_and_param[1]<<std::endl;
	}
}
void BreakRunLuaScript(){
	/*std::cout<<"Are you sure interrupt run the lua file?(yes/no)"<<std::endl;
	std::cout<<"DJI_AUTO>"
	std::string input;
	getline(std::cin,input);
	if(input.compare("yes")==0)*/
	LuaParser::LuaInterruptRuning("Console run break cmd.");
}
void RunLuaScript(void){
	if(_cmd_and_param[1].empty()){
		std::cout<<"Run CMD need a param for lua script file path"<<std::endl;
		return;
	}
	std::ifstream lua_file_handle(_cmd_and_param[1]);
	if (!lua_file_handle.is_open()){
		std::cout<<"lua script file open err"<<std::endl;
		return;
	}
	lua_file_handle.close();

	if(_lua_parser == nullptr){
		std::cout<<"Lua script parser is not exist"<<std::endl;
		return;
	}
	if(LuaParser::LuaScriptThreadRunning()){
		std::cout<<"There is a running lua script thread ,need waiting or break it."<<std::endl;
		return;
	}
	// creat a new thread for run user's lua script 
	_lua_parser->LuaScriptOpenAndRun(_cmd_and_param[1],true);
}
/*
*	load a .so lib for user payload control
*/
void LoadPayloadPlugin(){
	if(_cmd_and_param[1].empty()){
		DWAR("Load CMD need a param for dynamic lib path.");
		return;
	}
	// open the user payload control plugin .so  
	dynamic_lib_handler=dlopen(_cmd_and_param[1].c_str(),RTLD_NOW);
	if(!dynamic_lib_handler){
		DWAR("Load "+_cmd_and_param[1]+" dynamic lib err!");
	}
	// function handler
	typedef PayloadBase* (*payload_creat)(void);
	payload_creat _creat_func;
	
	// get the user's dynamic payload control lib. 
	_creat_func=(payload_creat)dlsym(dynamic_lib_handler,"PayloadInstance");
	//get payload plugin instance	
	_payload_base=_creat_func();
	
	if(!_payload_base->init()){
		DWAR("Init Payload control dynamic lib err!");
	}
}
void ExitSystemCMD(void){
	main_thread_need_exit=true;
}
void HelpCommandCMD(void);
//cmd function pointer
typedef void (*cmdfunction)(void);
typedef struct CMDFUNCTION{
	char cmd_name[10];
	cmdfunction cmd_function;
}CMD;
const CMD cmd_table[]={
	{"state",PrintFlightStatusCMD},
	{"config",PrintConfigMsgCMD},
	{"run",RunLuaScript},
	{"break",BreakRunLuaScript},
	{"load",LoadPayloadPlugin},
	{"photo",ShootPhoto},
	{"video",RunVideo},
	{"gimbal",SetGimbal},
	{"help",HelpCommandCMD},
	{"exit",ExitSystemCMD}
}; 
const char* cmd_description[]={
	"\t\tPrint the Vehicle and gimbal status.",
	"\t\tPrint the input config message in config.lua file.",
	"\t\trun the external lua script file ,need a *.lua file path as a argument.",
	"\t\tinterrupt run the lua script ",
	"\t\tload the user's Payload control plugin,need a *.so file path as a argument.",
	"\t\ttake a photo",
	"\t\tcontrol video start or stop record,need (start|stop)as a argument",
	"\t\tset gimbal angle ,e.g.(gimbal angle 0 10 0)",
	"\t\tprint this help message.",
	"\t\texit the application."
};
void HelpCommandCMD(void){
	for(int i=0; i<(sizeof(cmd_table)/sizeof(cmd_table[0]));  i++){
		std::cout<<" "<<cmd_table[i].cmd_name<<cmd_description[i]<<std::endl;
	}
}

/*
*	used to split the user input to cmd and param
*/
void splitCMDAndParam(std::string& input_stream){
	//clear cmd and param vector
	_cmd_and_param.clear();
	std::string::size_type pos1,pos2;
	// find the first not space position ,from 0 pos
	pos1=input_stream.find_first_not_of(" ",0);
	// find the space from pos1 position
	pos2=input_stream.find(" ",pos1); 
	while(pos1 != std::string::npos){
		_cmd_and_param.push_back(input_stream.substr(pos1,pos2-pos1));
		// find the next  not space pos ,from pos2
		pos1=input_stream.find_first_not_of(" ",pos2); 
		pos2=input_stream.find(" ",pos1);
		if(pos2 == std::string::npos){
			pos2=input_stream.size(); // last pos
		}
	}
}
/*
*	system entry function
*/
int main(int argc, char** argv){
	std::string config_file_path=argv[1];
	autopilot_system_init(config_file_path);
	std::string input;	

	std::cout<<"DJI_AUTO>";
	while(!main_thread_need_exit){
		//clear input		
		input.clear();
		//get string  form stand input
		getline(std::cin,input); // read a line input include space
		if(input.empty()){
			std::cout<<"DJI_AUTO>";		
			usleep(100000);//100ms
			continue;
		}
		
		//split cmd and param
		splitCMDAndParam(input);	
		int i=0;
		for(i=0; i<(sizeof(cmd_table)/sizeof(cmd_table[0]));  i++){
			if(_cmd_and_param[0].compare(cmd_table[i].cmd_name) == 0){
				//call the cmd function
				cmd_table[i].cmd_function();
				break;
			}
		}
		if(i == (sizeof(cmd_table)/sizeof(cmd_table[0]))){
			std::cout<<"Input CMD: "<<_cmd_and_param[0]<<" is't support yet."<<std::endl;
		}
		std::cout<<"DJI_AUTO>";
	}
	autopilot_system_exit();
	return 0;
}
