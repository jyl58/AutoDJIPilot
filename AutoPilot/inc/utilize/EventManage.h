#pragma once
#include <ev.h>
#include <thread>

class EventManage{
public:
	EventManage()=delete;
	~EventManage();
	static bool EventManageInit();
	static void EventManageExit();
private:
	static void EventManageThread();
	/*ev*/
	static struct ev_loop* _ev_loop;
	/*thread*/
	static std::thread* _event_manage_thread;
};
