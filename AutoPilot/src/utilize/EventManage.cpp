#include "EventManage.h"
#include "Message.h"

std::thread* EventManage::_event_manage_thread=nullptr;
struct ev_loop* EventManage::_ev_loop=ev_default_loop(0);

EventManage::~EventManage(){
	EventManageExit();
}
bool EventManage::EventManageInit(){
	
	_event_manage_thread=new std::thread(&EventManage::EventManageThread);
	if(_event_manage_thread==nullptr){
		DERR("Creat event manage err.")
		return false;
	}
	return true;
}
void EventManage::EventManageThread(){
	ev_run(_ev_loop);
}
void EventManage::EventManageExit(){
	if(_event_manage_thread != nullptr){
		ev_break(_ev_loop,EVBREAK_ALL);
		delete _event_manage_thread;
		_event_manage_thread=nullptr;
	}
}
