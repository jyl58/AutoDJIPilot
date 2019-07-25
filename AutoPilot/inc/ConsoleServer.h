/*
* @file ConsoleServer.h
* @author: JYL
* @email:jiyingliang369@126.com
* @date: 2019.06.26
*
*/
#pragma once
#include <string>
#include <thread>
#include <mutex>
#include <ev.h>
#include <sys/socket.h>
#include <netdb.h>
#include <vector>

#define SERVICE_PORT "63210"
#define TCP_BUFFER_MAX_SIZE 2048

typedef struct SOCKET_COONECT{
	ev_io	_tcp_talk;
	int index;
}socket_connect_t;
class ConsoleServer{
public:
	ConsoleServer();
	~ConsoleServer();
	bool ConsoleServerInit();

private:
	static void ConsoleServerListenThread();
	static void tCPAcceptCallback(struct ev_loop* main_loop, struct ev_io* sock_w,int events);
	static void tCPRead(struct ev_loop* main_loop, struct ev_io* client_r,int events);
	static void tCPWrite(struct ev_loop* main_loop, struct ev_io* client_w,int events);
	static void closeLinkAndStopIoEvent(struct ev_loop* main_loop,ev_io* need_release_io);

	static std::thread* _console_server_thread;
	/*ev*/
	static struct ev_loop* _ev_loop;
	static ev_io _tcp_listen;
	static std::vector<socket_connect_t*> _connect_list;

	/*socket*/
	static int _socket_fd;
	static int _talk_link_fd;
	const char* _port=SERVICE_PORT;
	/*cmd and parm*/
	std::vector<std::string>_cmd_and_param;
	

};
