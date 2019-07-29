#pragma once
#include <thread>
#include <mutex>
#include <ev.h>
#include "mavlink.h"
#include "FlightCore.h"
#include "InterfaceSerial.h"

#define SYSTEM_ID 100
#define COMPONENT_ID 101
#define MAVLINK_CHANNEL_0 0
#define GCS_SYSTEM_ID 200
#define GCS_COMPONENT_ID 201
class MavlinkRouter{
public:
	MavlinkRouter(){}
	~MavlinkRouter();
	static bool MavlinkRouterInit(const char *serial_port_name,int baudrate);
	static void sendHeartbeat();
	static void sendLocation(double lat,double lon,float relative_alt,float vx,float vy,float vz);
	static void stopMAVlinkThread();
	/* flight core*/
	//static FlightCore* _flight_core;
private:
	static void mavlinkRouterReadThread();
	static void mavlinkRouterSendThread();
	static void handle_mavlink_message(const mavlink_message_t &mavlink_msg);
	static void _handleHeartbeat(const mavlink_message_t &mavlink_msg);
	static void _handleCommandLong(const mavlink_message_t &mavlink_msg);
	static void _one_hz_timer_callback(struct ev_loop* ev_loop, ev_timer *timer,int event);
	
	/*ev*/
	static struct ev_loop* _mvalink_router_ev_loop;
	static ev_timer _1hz_loop;
	
	/*mavlink process thread */	
	static std::thread* _mavlink_router_read_thread;
	static std::mutex* _mavlink_router_read_thread_mutex;
	static bool _mavlink_thread_need_exit;

	/*serial handle*/
	static InterfaceSerial* _mavlink_serial;
};
