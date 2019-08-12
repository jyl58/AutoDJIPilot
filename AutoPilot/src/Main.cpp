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
#include <sstream>
#include <limits>
#include "Commander.h"
#include "Message.h"
/*
*	global var for message print function std::cout or socket fd
*	pre set the printfd to -1 for std::cout
*/
int SocketPrintFd=-1;
//app logo
const std::string AutoDjiLogo="DJI_Auto>";

bool main_thread_need_exit=false;
/*
*	system entry function
*/
int main(int argc, char** argv){
	//init commander system
	try{
		Commander::AutopilotSystemInit(argv[1]);
	}catch(const std::string& err){
		//print err msg
		std::cout<<"[ERR]Commander init err: "<<err<<std::endl;
		//delete the new memery
		Commander::AutopilotSystemExit();
		//exit run
		exit(1);
	}
	std::string input;	
	std::cout<<AutoDjiLogo;
	while(!Commander::main_thread_need_exit){
		//clear input		
		input.clear();
		//get string  form stand input
		getline(std::cin,input); // read a line input include space
		if(input.empty()){
			std::cout<<AutoDjiLogo;	
			continue;
		}
		//split cmd and param
		if(!Commander::splitCMDAndParam(input)){
			std::cout<<"Input command format err."<<std::endl;
			std::cout<<AutoDjiLogo;
			continue;	
		}
		// run cmd
		try{
			Commander::RunCommand();
			std::cout<<AutoDjiLogo;
		}catch(const std::string& err){
			//print err msg
			std::cout<<"[ERR]Run command err: "<<err<<std::endl;
			//go on next 
			continue;
		}
	}
	//exit app
	Commander::AutopilotSystemExit();
	return 0;
}
