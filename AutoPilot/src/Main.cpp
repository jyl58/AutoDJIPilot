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

bool main_thread_need_exit=false;
/*
*	system entry function
*/
int main(int argc, char** argv){
	Commander::AutopilotSystemInit(argv[1]);
	
	std::string input;	
	std::ostringstream outMsg(std::ios::app);
	std::cout<<"DJI_Auto>";
	while(!Commander::main_thread_need_exit){
		//clear input		
		input.clear();
		//get string  form stand input
		getline(std::cin,input); // read a line input include space
		if(input.empty()){
			std::cout<<"DJI_Auto>";	
			usleep(100000);//100ms
			continue;
		}
		
		//split cmd and param
		if(!Commander::splitCMDAndParam(input)){
			std::cout<<"Input command format err."<<std::endl;	
			std::cout<<"DJI_Auto>";
			continue;	
		}
		//clear	old message  buffer
		outMsg.str("");
		// run cmd
		Commander::RunCommand(outMsg);
		//print message to terminal
		std::cout<<outMsg.str()<<std::endl;
		std::cout<<"DJI_Auto>";
	}
	//exit app
	Commander::AutopilotSystemExit();
	return 0;
}
