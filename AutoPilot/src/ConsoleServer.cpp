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
#include "Message.h"
#include "EventManage.h"

ev_io ConsoleServer::_tcp_listen;
struct ev_loop* ConsoleServer::_ev_loop=ev_default_loop(0);
int ConsoleServer::_socket_fd;
std::vector<socket_connect_t*> ConsoleServer::_connect_list;

std::shared_ptr<ConsoleServer> ConsoleServer::getConsoleServerIntance(){
	return std::shared_ptr<ConsoleServer>(new ConsoleServer());
}
ConsoleServer::~ConsoleServer(){
	//close tcp link fd and stop io and delete _io_list memery 
	for (int i =0;i<_connect_list.size();i++){
		// close tcp link
		close(_connect_list[i]->_tcp_talk.fd);
		ev_io_stop(_ev_loop,&_connect_list[i]->_tcp_talk);
		delete _connect_list[i];
	}
	_connect_list.clear();
	ev_io_stop(_ev_loop,&_tcp_listen);
	//close the socket fd
	close(_socket_fd);
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
		DWAR(__FILE__,__LINE__,"Get addr info err.");
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
			DWAR(__FILE__,__LINE__,"Socket set Opt err.");
			return false;
		}
		//
		if(bind(_socket_fd,rp->ai_addr,rp->ai_addrlen) ==0 ){
			break;	
		}
		close(_socket_fd);
	}
	if(rp == NULL){
		DWAR(__FILE__,__LINE__,"There is no add info.");
		return false;
	}
	//free
	freeaddrinfo(result);
	
	//set no block
	long flags=fcntl(_socket_fd,F_GETFL);
	if( flags< 0){
		DWAR(__FILE__,__LINE__,"get Socket flags err.");
		return false;
	}
	flags |= O_NONBLOCK;
	if(fcntl(_socket_fd,F_SETFL,flags) < 0){
		DWAR(__FILE__,__LINE__,"Set Socket no block err.");
		return false;
	}
	//listen
	if(listen(_socket_fd,SOMAXCONN) ==-1){
		DWAR(__FILE__,__LINE__,"listen port err.");
		return false;
	}
	FLIGHTLOG("Socket is listenning.");
	//set the socket io event 
	ev_io_init(&_tcp_listen,ConsoleServer::tCPAcceptCallback,_socket_fd,EV_READ);
	ev_io_start(_ev_loop,&_tcp_listen);
	return true;
}
void 
ConsoleServer::tCPAcceptCallback(struct ev_loop* main_loop, ev_io* sock_w,int events){
	struct sockaddr_in link_in;
	socklen_t len =	sizeof(link_in);
	int link_fd=0;
	link_fd = accept(sock_w->fd,(struct sockaddr*)&link_in,&len);
	if(link_fd == -1){
		DWAR(__FILE__,__LINE__,"Socket accept err.");
		return;
	}
	FLIGHTLOG("There is a link comming.");
	/*if accept a link then creat a read and write event*/

	// no block		
	long flags=fcntl(link_fd,F_GETFL);
	if( flags< 0){
		return ;
	}
	flags |= O_NONBLOCK;
	if(fcntl(link_fd,F_SETFL,flags) < 0){
		return ;
	}
	//creat a new socket connect
	socket_connect_t* temp_connect=new socket_connect_t;
	//append to list
	_connect_list.push_back(temp_connect);
	//record the index in the io_list
	temp_connect->index=(_connect_list.size()-1);
	temp_connect->_tcp_talk.data=temp_connect;
	//init new event read client cmd
	ev_io_init(&temp_connect->_tcp_talk,ConsoleServer::tCPRead,link_fd,EV_READ);
	ev_io_start(main_loop,&temp_connect->_tcp_talk);
	//send logo
	send(link_fd,AutoDjiLogo.c_str(),AutoDjiLogo.size(),0);
}
void 
ConsoleServer::tCPRead(struct ev_loop* main_loop, struct ev_io* client_r,int events){
	if(EV_ERROR & events){
		DWAR(__FILE__,__LINE__,"EV event err.");
		return;
	}
	char buffer[TCP_BUFFER_MAX_SIZE]={0};
	ssize_t read_size=recv(client_r->fd,buffer,TCP_BUFFER_MAX_SIZE,0);	
	if(read_size < 0){
		DWAR(__FILE__,__LINE__,"Read socket err,err code: "+ std::to_string(errno));
		usleep(50000);	
		return ; 
	}
	//link closed by client
	if(read_size==0){
		closeLinkAndStopIoEvent(main_loop,client_r);
		FLIGHTLOG("Disconnect link by client close.");
		return;
	}
	
	std::string rc_input(buffer);
	//response enter("\n")
	if(rc_input.compare("\n")==0){
		send(client_r->fd,AutoDjiLogo.c_str(),AutoDjiLogo.size(),0);
		return;	
	}
	//erase the last enter symbal("\n")
	rc_input=rc_input.substr(0,rc_input.find_last_not_of("\n")+1);
	if(!Commander::splitCMDAndParam(rc_input)){
		std::string msg("Input command format err.\n");
		send(client_r->fd,msg.c_str(),msg.size(),0);
		send(client_r->fd,AutoDjiLogo.c_str(),AutoDjiLogo.size(),0);
		return;
	}
	// run cmd
	try{
		Commander::RunCommand(client_r->fd);
	}catch(const std::string& err){
		std::string err_message="[ERR]Run err:"+err;
		send(client_r->fd,err_message.c_str(),err_message.size(),0);
	}
	send(client_r->fd,AutoDjiLogo.c_str(),AutoDjiLogo.size(),0);
	if(Commander::tcp_link_need_disconnect){
		closeLinkAndStopIoEvent(main_loop,client_r);
		FLIGHTLOG("Disconnect link by cmd.");
	}
}
void 
ConsoleServer::closeLinkAndStopIoEvent(struct ev_loop* main_loop,ev_io* need_release_io){
	socket_connect_t *curent_connect=(socket_connect_t*)need_release_io->data;
	// close tcp link
	close(need_release_io->fd);
	//stop io poll event
	ev_io_stop(main_loop,need_release_io);
	//delete new heap 
	delete _connect_list[curent_connect->index];
	// remove from list
	_connect_list.erase(_connect_list.begin()+curent_connect->index);
		
}
