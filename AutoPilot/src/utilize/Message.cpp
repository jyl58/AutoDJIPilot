/*
* @file Message.cpp
* @author: JYL
* @email:jiyingliang369@126.com
* @date: 2019.08.1
*
*/
#include "Message.h"
void DERR(std::string warn_file,int line ,std::string msg){			
	std::string err_message=warn_file.substr(warn_file.find_last_of("/")+1)+": "+std::to_string(line)+": "+ msg;
	FLIGHTLOG("[ERR]"+err_message);										
	throw warn_file.substr(warn_file.find_last_of("/")+1)+": "+std::to_string(line)+": "+ msg;
}	
void DWAR(std::string warn_file,int line ,std::string msg,int fd){
														
	std::string warn_message="[WAR]"+warn_file.substr(warn_file.find_last_of("/")+1)+": "+std::to_string(line)+": "+ msg;
	NOTICE_MSG(fd,warn_message);
	FlightLog::writeLogBuffer(warn_message);																
}
void NOTICE_MSG(int fd,std::string msg){
	if(fd == -1){							
		std::cout<<msg<<std::endl;			
	}else{									
		send(fd,msg.c_str(),msg.size(),0);	
		send(fd,"\n",1,0);					
	}		
}
void LOGO(int fd,std::string msg){
	if(fd == -1){							
		std::cout<<msg<<std::flush;		
	}else{									
		send(fd,msg.c_str(),msg.size(),0);
	}		
}

