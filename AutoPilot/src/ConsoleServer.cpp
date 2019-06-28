/*
* @file ConsoleServer.cpp
* @author: JYL
* @email:jiyingliang369@126.com
* @date: 2019.06.27
*
*/
#include <sys/unistd.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sstream>
#include <fcntl.h>
#include <stdio.h>
#include <netinet/in.h>
#include "ConsoleServer.h"
#include "Commander.h"
ev_io ConsoleServer::_tcp_listen;
ev_io ConsoleServer::_tcp_talk;
struct ev_loop* ConsoleServer::_ev_loop=ev_default_loop(0);
std::thread* ConsoleServer::_console_server_thread=nullptr;
int ConsoleServer::_socket_fd;
int ConsoleServer::_talk_link_fd;
const std::string ConsoleServer::console_server_logo="DJI_Auto>";

ConsoleServer::ConsoleServer(){

}

ConsoleServer::~ConsoleServer(){
	if(_console_server_thread!=nullptr){
		//STOP LIBEV
		ev_break(_ev_loop,EVBREAK_ALL);
		_console_server_thread->join();
		delete _console_server_thread;
		_console_server_thread=nullptr;
	}
}
bool 
ConsoleServer::ConsoleServerInit(){
	//creat socket and bind and listen
	struct addrinfo hits;
	struct addrinfo* result,*rp;
	memset(&hits,0,sizeof(struct addrinfo));
	hits.ai_family	=AF_UNSPEC;
	hits.ai_socktype=SOCK_STREAM;
	hits.ai_flags	=AI_PASSIVE;
	int ret=getaddrinfo(NULL,_port,&hits,&result);
	if(ret!=0){
		DWAR("Get addr info err.");
		return false;
	}
	for(rp=result;rp!=NULL;rp=rp->ai_next){
		_socket_fd=socket(rp->ai_family,rp->ai_socktype,rp->ai_protocol);
		if(_socket_fd == -1){
			continue;
		}
	
		int reuse=1;
		struct linger so_linger;
		so_linger.l_onoff=1;
		so_linger.l_linger=1;
		if(setsockopt(_socket_fd,SOL_SOCKET,SO_REUSEPORT,(void*)&reuse,sizeof(reuse)) <0 || 
		   setsockopt(_socket_fd,SOL_SOCKET,SO_LINGER,&so_linger,sizeof(so_linger)) < 0){
			DWAR("Socket set Opt err.");
			return false;
		}
		//
		if(bind(_socket_fd,rp->ai_addr,rp->ai_addrlen) ==0 ){
			break;	
		}
		close(_socket_fd);
	}
	if(rp == NULL){
		DWAR("There is no add info.");
		return false;
	}
	//free
	freeaddrinfo(result);
	
	//set no block
	long flags=fcntl(_socket_fd,F_GETFL);
	if( flags< 0){
		DWAR("get Socket flags err.");
		return false;
	}
	flags |= O_NONBLOCK;
	if(fcntl(_socket_fd,F_SETFL,flags) < 0){
		DWAR("Set Socket no block err.");
		return false;
	}
	//listen
	if(listen(_socket_fd,SOMAXCONN) ==-1){
		DWAR("listen port err.");
		return false;
	}
	FLIGHTLOG("Socket is listenning.");
	FLIGHTLOG("creat a console server thread.");
	//creat a new thread
	if(_console_server_thread != nullptr){
		_console_server_thread->join();
		delete _console_server_thread;
		_console_server_thread=nullptr;
	}
	_console_server_thread=new std::thread(&ConsoleServer::ConsoleServerListenThread);
	return true;
}
void 
ConsoleServer::ConsoleServerListenThread(){
	//init a listen poll event 
	ev_io_init(&_tcp_listen,ConsoleServer::tCPAcceptCallback,_socket_fd,EV_READ);
	ev_io_start(_ev_loop,&_tcp_listen);
	ev_run(_ev_loop);
}
void 
ConsoleServer::tCPAcceptCallback(struct ev_loop* main_loop, ev_io* sock_w,int events){
	struct sockaddr_in link_in;
	socklen_t len =	sizeof(link_in);
	_talk_link_fd = accept(sock_w->fd,(struct sockaddr*)&link_in,&len);
	if(_talk_link_fd == -1){
		DWAR("Socket accept err.");
		return;
	}
	FLIGHTLOG("There is a link comming.");
	/*if accept a link then creat a read and write event*/

	// no block		
	long flags=fcntl(_talk_link_fd,F_GETFL);
	if( flags< 0){
		return ;
	}
	flags |= O_NONBLOCK;
	if(fcntl(_talk_link_fd,F_SETFL,flags) < 0){
		return ;
	}
	// init new event read client cmd
	ev_io_init(&ConsoleServer::_tcp_talk,ConsoleServer::tCPRead,_talk_link_fd,EV_READ);
	ev_io_start(main_loop,&ConsoleServer::_tcp_talk);
	//send logo
	send(_talk_link_fd,console_server_logo.c_str(),console_server_logo.size(),0);
		
}
void 
ConsoleServer::tCPRead(struct ev_loop* main_loop, struct ev_io* client_r,int events){
	if(EV_ERROR & events){
		DWAR("Event err.");
		return;
	}
	char buffer[TCP_BUFFER_MAX_SIZE]={0};
	ssize_t read_size=recv(client_r->fd,buffer,TCP_BUFFER_MAX_SIZE,0);	
	if(read_size < 0){
		DWAR("Read socket err,err code: "+ std::to_string(errno));
		usleep(50000);	
		return ; 
	}
	//link closed by client
	if(read_size==0){
		close(client_r->fd);
		ev_io_stop(main_loop,&ConsoleServer::_tcp_talk);
		FLIGHTLOG("Disconnect link by client.");
		return;
	}
	
	std::string rc_input(buffer);
	if(rc_input.empty()){
		send(client_r->fd,console_server_logo.c_str(),console_server_logo.size(),0);
		return;	
	}
	
	Commander::splitCMDAndParam(rc_input);
	
	std::ostringstream outMsg(std::ios::app);
	outMsg.clear();
	// run cmd
	Commander::RunCommand(outMsg);
	//send to client
	outMsg.seekp(0,std::ios::end);
	int len=outMsg.tellp();
	outMsg.seekp(0,std::ios::beg);
	//send cmmd return message to tcp
	send(client_r->fd,(void*)outMsg.str().c_str(),len,0);
	if(Commander::main_thread_need_exit){
		close(client_r->fd);
		ev_io_stop(main_loop,&ConsoleServer::_tcp_talk);
		FLIGHTLOG("Disconnect link.");
	}
}

