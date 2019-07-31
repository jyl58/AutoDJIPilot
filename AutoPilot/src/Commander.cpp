/*
* @file Commander.cpp
* @author: JYL
* @email:jiyingliang369@126.com
* @date: 2019.06.27
*
*/
#include <fstream>
#include <sstream>
#include <dlfcn.h>
#include "Commander.h"
#include "EventManage.h"

ConsoleServer* Commander::_console_server=nullptr;	
LinuxSetup* Commander::_linux_setup=nullptr;
FlightCore* Commander::_flight_core=nullptr;
LuaParser* Commander::_lua_parser=nullptr;
PayloadBase* Commander::_payload_base=nullptr;
void* Commander::dynamic_lib_handler=nullptr;
bool Commander::main_thread_need_exit=false;
bool Commander::tcp_link_need_disconnect=false;
std::vector<std::string> Commander::_cmd_and_param;
std::string  Commander::_env_home;
//note: name length is little 10 char
const command_function_t Commander::cmd_table[]={
	{"state",Commander::PrintFlightStatusCMD},
	{"config",Commander::PrintConfigMsgCMD},
	{"run",Commander::RunLuaScript},
	{"pause",Commander::PauseRunLuaScript},
	{"go",Commander::GoOnRunLuaScript},
	{"break",Commander::BreakRunLuaScript},
	{"load",Commander::LoadPayloadPlugin},
	{"photo",Commander::ShootPhoto},
	{"video",Commander::RunVideo},
	{"gimbal",Commander::SetGimbal},
	{"zoom",Commander::ZoomCamera},
	{"help",Commander::HelpCommandCMD},
	{"exit",Commander::ExitSystemCMD}
}; 

const char* Commander::cmd_description[]={
	"\tPrint the Vehicle and gimbal status.",
	"\tPrint the input config message in config.lua file.",
	"\tRun the external lua script file ,need a *.lua file path as a argument.",
	"\tPause the Runing lua script.",
	"\tContinue run the lua script.",
	"\tInterrupt run the lua script.",
	"\tLoad the user's Payload control plugin,need a *.so file path as a argument.",
	"\tTake a photo use dji camera.",
	"\tControl dji video start or stop record,need (start|stop)as a argument.",
	"\tSet gimbal angle this angle relevtive vehicle's head (-pi,pi],e.g.(gimbal angle 0 10 0).",
	"\tZoom camera,just support z30 ,need(pos|speed|step)as a subcmd.(pos [100,3000] | speed [-100,100] |step [].",
	"\tPrint this help message.",
	"\tExit the application."
};
void 
Commander::AutopilotSystemInit(const std::string& config_file_path){
	//first  init log sub thread for record sys message
	if(!FlightLog::FlightLogInit()){
		DERR("Flight log init err!");
		exit(1);
	}
	// creat a lua parser.
	_lua_parser		=new LuaParser();
	if(_lua_parser == nullptr){
		DERR("Creat lua parser err!");
		exit(1);
	}
	//creat a liux enviroment init instance.
	_linux_setup	=new LinuxSetup(_lua_parser,config_file_path);
	if(_linux_setup == nullptr)
		exit(1);
#ifdef OFFLINE_DEBUG
#else		
	int try_count=0;
	while(try_count<10){
		if(_linux_setup->initVehicle()){
			break;
		}
		//waiting fc init complete. max waiting time is 120 s
		sleep(12);
		try_count++;
		FLIGHTLOG("Try " + std::to_string(try_count+1) +"th to connect the vehilce.");
	}
	if(try_count>=10){
		delete _linux_setup;
		_linux_setup=nullptr;
		DERR("Connect to vehicle err!");
		exit(1);
	}
	
	_flight_core	=new FlightCore();
	if(_flight_core == nullptr ){
		DERR("Creat flight core err!");
		exit(1);
	}
	if(!_flight_core->flightCoreInit(_linux_setup->getVehicle())){
		delete _flight_core;
		_flight_core=nullptr;
		DERR("Flight core init err!");
		exit(1);
	}
	LuaInterface::_flight_core =_flight_core;
	//MavlinkRouter::_flight_core=_flight_core;
#endif
    // int flight mavlink router thread	
	if (!MavlinkRouter::MavlinkRouterInit(_linux_setup->getMAVlinkDevPort().c_str(),_linux_setup->getMAVlinkDevPortBaudrate())){
		DERR("Mavlink router init err!");
		exit(1);	
	}
	//creat a consoler server for remote cmd 
	_console_server=new ConsoleServer();
	if(_console_server == nullptr){
		DERR("Flight Console creat err!");
		exit(1);
	}
	if(!_console_server->ConsoleServerInit()){
		delete _console_server;
		_console_server=nullptr;
		DERR("Flight Console Server init err!");
		exit(1);
	}
	if(!EventManage::EventManageInit()){
		DERR("Event Manage init err!");
		exit(1);
	}
	_env_home=getenv("HOME");
}
void 
Commander::AutopilotSystemExit(){
	// the order of decontructor is important
	//exit the lua parser thread
	if (_lua_parser != nullptr ){
		delete _lua_parser;
		_lua_parser=nullptr;
	}
	//exit flight core thread
	if(_flight_core != nullptr){
		delete _flight_core;
		_flight_core= nullptr;
	}
	//delete the setup instance
	if(_linux_setup != nullptr) {
		delete _linux_setup;
		_linux_setup= nullptr;
	}
	//exit the mavlink router thread
	MavlinkRouter::stopMAVlinkThread();
	//stop the console sever event loop
	if( _console_server!= nullptr){
		delete _console_server;
		_console_server=nullptr;
	}
	//exit the event manage thread
	EventManage::EventManageExit();
	//last: exit the log thread
	FlightLog::FlightLogStop();
	//close the dynamic lib
	if(dynamic_lib_handler != nullptr)
		dlclose(dynamic_lib_handler);
}
/*
*	used to split the user input to cmd and param
*	return : true if split successful ,false 
*/
bool
Commander::splitCMDAndParam(const std::string& input_stream){
	//clear cmd and param vector
	_cmd_and_param.erase(_cmd_and_param.begin(),_cmd_and_param.end());
	std::string::size_type pos1,pos2;
	// find the first not space position ,from 0 pos
	pos1=input_stream.find_first_not_of(" ",0);
	// Do not find a no space character return	
	if(pos1==std::string::npos)
		return false;
	while(pos1 != std::string::npos){
		// find the space from pos1 position
		pos2=input_stream.find(" ",pos1); 
		if(pos2 == std::string::npos){
			pos2=input_stream.size(); // last pos
		}
		_cmd_and_param.push_back(input_stream.substr(pos1,pos2-pos1));
		// find the next  not space pos ,from pos2
		pos1=input_stream.find_first_not_of(" ",pos2);
	}
	return true;
}
void 
Commander::HelpCommandCMD(int print_fd){
	for(int i=0; i<(sizeof(cmd_table)/sizeof(cmd_table[0]));  i++){
		std::string cmd_name=cmd_table[i].cmd_name;
		NOTICE_MSG(print_fd,"\t" + cmd_name + cmd_description[i]);
	}
}
void 
Commander::ExitSystemCMD(int print_fd){
	main_thread_need_exit=true;
}
void 
Commander::PrintConfigMsgCMD(int print_fd){
	if(_linux_setup==nullptr){
		NOTICE_MSG(print_fd,"config file parse err!");
		return ;
	}
	NOTICE_MSG(print_fd,"**********Config Msg****************");
	NOTICE_MSG(print_fd,"DJI app id:           	    " + std::to_string(_linux_setup->getDJIAppId()));
	NOTICE_MSG(print_fd,"DJI app key:          	    " + _linux_setup->getDJIAppKey());
	NOTICE_MSG(print_fd,"DJI device port:      	    " + _linux_setup->getDJIDevicePort());
	NOTICE_MSG(print_fd,"DJI device baudrate:  	    " + std::to_string(_linux_setup->getDJIDevicePortBaudrate()));
	NOTICE_MSG(print_fd,"MAV device port:           " + _linux_setup->getMAVlinkDevPort());
	NOTICE_MSG(print_fd,"MAV device port baudrate:  " + std::to_string(_linux_setup->getMAVlinkDevPortBaudrate()));
	NOTICE_MSG(print_fd,"************************************");
}
void 
Commander::PrintFlightStatusCMD(int print_fd){
	if(_flight_core==nullptr){
		NOTICE_MSG(print_fd,"Flight Core is't run!");
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
	
	NOTICE_MSG(print_fd,"**********Flight Status**************");
	NOTICE_MSG(print_fd,"Flight Mode:    "+ std::to_string(display_mode));
	NOTICE_MSG(print_fd,"Status:         "+ std::to_string(flightStatus));
	NOTICE_MSG(print_fd,"Battery Info:   "+ std::to_string(battery_info.voltage*0.001)+"(V), "+
											std::to_string(battery_info.current*0.001)+"(A),"+
											std::to_string(battery_info.percentage)+"%");
											
	NOTICE_MSG(print_fd,"Positon(LLA):   "+ std::to_string(current_lat_lon.latitude*RAD2DEG)+"(deg), "+
											std::to_string(current_lat_lon.longitude*RAD2DEG)+"(deg), "+
											std::to_string(altitude_fusioned)+"(m)");
											
	NOTICE_MSG(print_fd,"Head:           "+ std::to_string(vehicle_head)+"(deg)");
	
	NOTICE_MSG(print_fd,"GPS Message:    "+ std::to_string(gps_details.hdop)+"(hdop), "+
											std::to_string(gps_details.hacc)+"(hps), "+
											std::to_string((int)gps_details.fix) +"(fix), "+
											std::to_string(gps_details.usedGPS)+"(number), "+
											std::to_string(gps_signal_level)+"(sig(0~5))");
											
	NOTICE_MSG(print_fd,"Velocity(m/s):  " +std::to_string(velocity.data.x)+", "+std::to_string(velocity.data.y)+", "+std::to_string(velocity.data.z));
	if(gimbal_status.mountStatus){
		uint8_t gimbal_mode=_flight_core->getGimbalMode();
		NOTICE_MSG(print_fd,"Gimbal Mode:	   "+std::to_string(gimbal_mode));
		NOTICE_MSG(print_fd,"Gimbal angle(rpy):"+std::to_string(gimbal_angle.y)+"(deg), "+std::to_string(gimbal_angle.x)+"(deg), "+std::to_string(gimbal_angle.z)+"(deg)");
	}
	NOTICE_MSG(print_fd,"************************************");
}
void 
Commander::ZoomCamera(int print_fd){
	if(_flight_core==nullptr){
		NOTICE_MSG(print_fd,"Flight Core is't run!");
		return;
	}
	if(_cmd_and_param.at(1).empty()){
		NOTICE_MSG(print_fd,"Zoom CMD need a subcmd as argument.");
		return;
	}
	if(_cmd_and_param.at(1).compare("pos")){
		if(_cmd_and_param.at(2).empty()){
			NOTICE_MSG(print_fd,"Zoom CMD need a subcmd as argument.");
			return;
		}
		_flight_core->djiCameraZoomByPos((uint16_t)std::stoi(_cmd_and_param.at(2)));
	}else if(_cmd_and_param.at(1).compare("speed")){
		if(_cmd_and_param.at(2).empty()){
			NOTICE_MSG(print_fd,"Zoom speed need a value as argument(e.g. zoom speed 50).");
			return;
		}
		_flight_core->djiCameraZoomBySpeed((int16_t)std::stoi(_cmd_and_param.at(2)));
	}else if (_cmd_and_param.at(1).compare("step")){
		_flight_core->djiCameraZoomBystep((int16_t)std::stoi(_cmd_and_param.at(2)));
	}else{
		NOTICE_MSG(print_fd,"Don't support zoom's subcmd: "+_cmd_and_param.at(1));
	}
}
void 
Commander::LoadPayloadPlugin(int print_fd){
	if(_cmd_and_param.at(1).empty()){
		DWAR("Load CMD need a param for dynamic lib path.",print_fd);
		return;
	}
	//close if there is a plugin
	if(dynamic_lib_handler != nullptr)
		dlclose(dynamic_lib_handler);

	// open the user payload control plugin .so  
	dynamic_lib_handler=dlopen(_cmd_and_param.at(1).c_str(),RTLD_NOW);
	if(!dynamic_lib_handler){
		DWAR("Load "+_cmd_and_param.at(1)+" dynamic lib err!",print_fd);
	}
	// function handler
	typedef PayloadBase* (*payload_creat)(void);
	payload_creat _creat_func;
	
	// get the user's dynamic payload control lib. 
	_creat_func=(payload_creat)dlsym(dynamic_lib_handler,"PayloadInstance");
	//get payload plugin instance	
	_payload_base=_creat_func();
	
	if(!_payload_base->init()){
		DWAR("Init Payload control dynamic lib err!",print_fd);
	}
}
void 
Commander::RunLuaScript(int print_fd){
	if(_cmd_and_param.size()<2){
		NOTICE_MSG(print_fd,"Run CMD need a param for lua script file path.");
		return;
	}
	//auto add file abslutely file path
	std::string file_absolute_path=_cmd_and_param.at(1);
	if(file_absolute_path.find("/")==std::string::npos){
		//just give a file name,so find in default directory
		std::string default_bin_path= _env_home+DEFAULT_BIN_DIRECTORY+file_absolute_path;
		if(access(default_bin_path.c_str(),F_OK) == -1){
			NOTICE_MSG(print_fd,"Do not find the "+file_absolute_path+" in default bin directory.");
			return;
		}
		file_absolute_path=default_bin_path;
	}
	std::ifstream lua_file_handle(file_absolute_path);
	if (!lua_file_handle.is_open()){
		NOTICE_MSG(print_fd,"Lua script file open err.");
		return;
	}
	lua_file_handle.close();

	if(_lua_parser == nullptr){
		NOTICE_MSG(print_fd,"Lua script parser is not exist.");
		return;
	}
	if(LuaParser::LuaScriptThreadRunning()){
		NOTICE_MSG(print_fd,"There is a running lua script thread ,need waiting or break it.");
		return;
	}
	// creat a new thread for run user's lua script 
	_lua_parser->LuaScriptOpenAndRun(file_absolute_path,print_fd,true);
}
void 
Commander::PauseRunLuaScript(int print_fd){
	LuaParser::LuaRunPause("console pause cmd");
}
void 
Commander::GoOnRunLuaScript(int print_fd){
	LuaParser::LuaRunGoOn("console continue cmd");
}
void 
Commander::BreakRunLuaScript(int print_fd){
	LuaParser::LuaInterruptRuning("Console run break cmd.");
}

void 
Commander::SetGimbal(int print_fd){
	if(_flight_core==nullptr){
		NOTICE_MSG(print_fd,"Flight Core is't run!");
		return;
	}
	TypeMap<TOPIC_GIMBAL_STATUS>::type		gimbal_status;
	_flight_core->getGimbalStatus(&gimbal_status);
	if(gimbal_status.mountStatus==0){
		NOTICE_MSG(print_fd,"Gimbal is not mount!");
		return;
	}
	if(_cmd_and_param.size()<2){
		NOTICE_MSG(print_fd,"gimbal CMD need a argument (angle|speed).");
		return;
	}

	if(_cmd_and_param.at(1).compare("angle") == 0){
		float roll_deg=_cmd_and_param.size()<3?  0 : std::stof(_cmd_and_param.at(2));
		float pitch_deg=_cmd_and_param.size()<4? 0 : std::stof(_cmd_and_param.at(3));
		float yaw_deg=_cmd_and_param.size()<5?   0 : std::stof(_cmd_and_param.at(4));
		_flight_core->djiSetGimbalAngle(roll_deg,pitch_deg,yaw_deg);
	}else if(_cmd_and_param.at(1).compare("speed") == 0){
		//TODO: add speed 	
	}else{
		NOTICE_MSG(print_fd,"Don't support gimbal's subcmd: "+_cmd_and_param.at(1));
	}
}
void 
Commander::RunVideo(int print_fd){
	if(_flight_core==nullptr){
		NOTICE_MSG(print_fd,"Flight Core is't run!");
		return;
	}
	if(_cmd_and_param.size()<2){
		NOTICE_MSG(print_fd,"video CMD need a argument (start|stop).");
		return;
	}
	if(_cmd_and_param.at(1).compare("start")==0){
		_flight_core->djiVideoStart();
		NOTICE_MSG(print_fd,"video start...");
	}else if (_cmd_and_param.at(1).compare("stop")==0){
		_flight_core->djiVideoStop();
		NOTICE_MSG(print_fd,"video stop");
	}else{
		NOTICE_MSG(print_fd,"Don't support video's subcmd: "+_cmd_and_param.at(1));
	}
}
void 
Commander::ShootPhoto(int print_fd){
	if(_flight_core==nullptr){
		NOTICE_MSG(print_fd,"Flight Core is't run!");
		return;
	}	
	_flight_core->djiShootPhoto();
}
void 
Commander::RunCommand(int print_fd){
	int cmd_index=0;
	for(cmd_index=0; cmd_index<(sizeof(cmd_table)/sizeof(cmd_table[0]));  cmd_index++){
		if(_cmd_and_param.front().compare(cmd_table[cmd_index].cmd_name) == 0){
			//call the cmd function
			cmd_table[cmd_index].cmd_function(print_fd);
			break;
		}
	}
	if(cmd_index == (sizeof(cmd_table)/sizeof(cmd_table[0]))){
		NOTICE_MSG(print_fd,"Input CMD: "+_cmd_and_param.front()+" is't support yet.");
	}
}
