/*
* @file FlightLog.cpp
* @author: JYL
* @email:jiyingliang369@126.com
* @date: 2019.06.10
*
*/
#include <ctime>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include "FlightLog.h"
#include "Message.h"

std::thread* FlightLog::_flight_log_thread=nullptr;
bool 		 FlightLog::_thread_need_exit=false;
std::mutex*  FlightLog::_write_log_mutex=nullptr;
std::string  FlightLog::_log_buff="";

FlightLog::FlightLog(){

}
FlightLog::~FlightLog(){
	FlightLogStop();
}
void FlightLog::FlightLogStop(){
	if(_flight_log_thread != nullptr){
		_thread_need_exit=true;
		// waiting the thread exit
		_flight_log_thread->join();	
		delete _flight_log_thread;
		_flight_log_thread=nullptr;
	}
	if(_write_log_mutex != nullptr){
		delete _write_log_mutex;
		_write_log_mutex = nullptr;
	}
}
bool FlightLog::FlightLogInit(){
	// first stop the running thread
	FlightLogStop(); 
	DDBUG("Log thread init");
	// creat a log mutex;
	_write_log_mutex=new std::mutex();
	if(_write_log_mutex == nullptr)
		return false;
	
	//creat a new thread
	_flight_log_thread= new std::thread(&FlightLog::writeLogThread);
	if(_flight_log_thread == nullptr)
		return false;
	return true;
}
void FlightLog::writeLogBuffer(const std::string& log_context){
	time_t now_raw_time;
	struct tm* timeinfo;
	time(&now_raw_time);
	timeinfo=localtime(&now_raw_time);
	char head[1024];	
	sprintf(head,"[%.2d:%.2d:%.2d]",timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec);
	std::string log_head(head);
	_write_log_mutex->lock();
	_log_buff.append(log_head + log_context+"\n");
	_write_log_mutex->unlock();
}

void FlightLog::writeLogBufferWithLabel(const std::string& log_context){
	std::string label;
	label.append("[WAR](");
	label.append(__FUNCTION__);
	label.append(")(");
	label.append(std::to_string(__LINE__));
	label.append(")");
	writeLogBuffer(label+log_context);
}
void FlightLog::writeLogThread(){
	//creat a file in current directory	
	time_t now_raw_time;
	struct tm* timeinfo;
	time(&now_raw_time);
	timeinfo=localtime(&now_raw_time);

	char temp_name[1024];
	sprintf(temp_name,"FlightLog-%d-%.2d-%.2d-%.2d:%.2d:%.2d.log",1900+timeinfo->tm_year, timeinfo->tm_mon, timeinfo->tm_mday,timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	std::string log_name(temp_name);
	
	//get current path 
	/*char  cwd[1024];
	if (getcwd(cwd, sizeof(cwd)) == NULL){
    	DERR("Error getting current directory.");
	}*/
	std::string log_path=getenv("HOME");
	log_path=log_path+LOG_PATH;
	//creat a log dirctory  at current directory if not exist
	if(access(log_path.c_str(),W_OK|R_OK) !=0 ){
		if(mkdir(log_path.c_str(), S_IRWXU | S_IXGRP |S_IRGRP |S_IROTH | S_IXOTH) !=0){
			DERR("Log directory creat fail.");
		}
	}
	// file absolute path
	std::string log_file_path=log_path+log_name;
	std::ofstream log_file_handle(log_file_path.c_str(),std::ofstream::out);
	if(!log_file_handle.is_open()){
		DERR("Open Flight Log err.");
		return;
	}
	FLIGHTLOG("Start run flight log Thread...");
	while(!_thread_need_exit){
		_write_log_mutex->lock();
		if (!_log_buff.empty()){
			// write log to file			
			log_file_handle<<_log_buff<<std::endl;
			//clear the log buffer
			_log_buff.clear();	
		}
		_write_log_mutex->unlock();
		
		sleep(1);// sleep 1s 
	}
	/* write the cache msg to file*/
	_write_log_mutex->lock();
	if (!_log_buff.empty()){
		// write log to file			
		log_file_handle<<_log_buff<<std::endl;
		//clear the log buffer
		_log_buff.clear();	
	}
	_write_log_mutex->unlock();
	log_file_handle.close();
}
