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
#include "Commander.h"
//app logo
const std::string AutoDjiLogo="DJI_Auto>";

bool main_thread_need_exit=false;
/*
*	system entry function
*/
int main(int argc, char** argv){
	Commander::AutopilotSystemInit(argv[1]);
	
	std::string input;	
	std::cout<<AutoDjiLogo;
	while(!Commander::main_thread_need_exit){
		//clear input		
		input.clear();
		//get string  form stand input
		getline(std::cin,input); // read a line input include space
		if(input.empty()){
			std::cout<<"\n"<<AutoDjiLogo;	
			usleep(100000);//100ms
			continue;
		}
		//split cmd and param
		if(!Commander::splitCMDAndParam(input)){
			std::cout<<"Input command format err."<<std::endl;	
			std::cout<<AutoDjiLogo;
			continue;	
		}
		// run cmd
		Commander::RunCommand();
		std::cout<<AutoDjiLogo;
	}
	//exit app
	Commander::AutopilotSystemExit();
	return 0;
}
