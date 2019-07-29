#pragma once
#include <ev.h>
#include <thread>

class EventManage{
public:
	EventManage(){}
	~EventManage();
	static bool EventManageInit();
	static void EventManageExit();
	static struct ev_loop* GetEvLoopHandler(){return _ev_loop;}
private:
	static void EventManageThread();
	/*ev*/
	static struct ev_loop* _ev_loop;
	/*thread*/
	static std::thread* _event_manage_thread;

};
