#pragma once
#include <thread>
#include <mutex>
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
	static bool MavlinkRouterInit(const char *serial_port_name);
	static void sendHeartbeat();
	static void sendLocation(double lat,double lon,float relative_alt,float vx,float vy,float vz);
	static void stopMAVlinkThread();
	/* flight core*/
	static FlightCore* _flight_core;
private:
	static void mavlinkRouterThread();
	static void handle_mavlink_message(const mavlink_message_t &mavlink_msg);
	static void _handleHeartbeat(const mavlink_message_t &mavlink_msg);
	static void _handleCommandLong(const mavlink_message_t &mavlink_msg);
	

	/*mavlink process thread */	
	static std::thread* _mavlink_router_thread;
	static std::mutex* _mavlink_tread_mutex;
	static bool _mavlink_thread_need_exit;

	/*serial handle*/
	static InterfaceSerial* _mavlink_serial;
};
