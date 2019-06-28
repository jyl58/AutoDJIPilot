/*
* @file FlightCore.h
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

struct socket_conn_t{
	struct ev_loop *loop;
	ev_io	read_w;
	//ev_io	write_w;
	ev_io	time_w;
};
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

	static std::thread* _console_server_thread;
	/*ev*/
	static struct ev_loop* _ev_loop;
	static ev_io _tcp_listen;
	static ev_io _tcp_talk;

	/*socket*/
	static int _socket_fd;
	static int _talk_link_fd;
	const char* _port=SERVICE_PORT;
	static const std::string console_server_logo;
	/*cmd and parm*/
	std::vector<std::string>_cmd_and_param;
	

};
