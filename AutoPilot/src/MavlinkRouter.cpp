#include "MavlinkRouter.h"
#include "Message.h"

std::thread* MavlinkRouter::_mavlink_router_read_thread=nullptr;
std::mutex*  MavlinkRouter::_mavlink_router_read_thread_mutex=nullptr;
//FlightCore*  MavlinkRouter::_flight_core=nullptr;
InterfaceSerial* MavlinkRouter::_mavlink_serial=nullptr;
bool MavlinkRouter::_mavlink_thread_need_exit=false;
struct ev_loop* MavlinkRouter::_mvalink_router_ev_loop=ev_default_loop(0);
ev_timer MavlinkRouter::_1hz_loop;

bool MavlinkRouter::MavlinkRouterInit(const char *serial_port_name,int baudrate){
	// first check running thread,then close it
	stopMAVlinkThread();
	
	_mavlink_serial=new InterfaceSerial(serial_port_name,baudrate);
	if(_mavlink_serial == nullptr){
		DERR("Creat serial pointer err.");	
		return false;
	}
	_mavlink_router_read_thread=new std::thread(&MavlinkRouter:: mavlinkRouterReadThread);
	if(_mavlink_router_read_thread == nullptr){
		DERR("Creat mavlink router read thread err.");	
		return false;
	}
	//set send ev thread 1hz herat beat
	ev_timer_init(&_1hz_loop,MavlinkRouter::_one_hz_timer_callback,1,1);
	ev_timer_start(_mvalink_router_ev_loop,&_1hz_loop);
	return true;
}
MavlinkRouter::~MavlinkRouter(){
	stopMAVlinkThread();
}

void MavlinkRouter::sendHeartbeat(){
	if(_mavlink_serial == nullptr){
		DERR(" MAVlink serial is nullptr.");
		return;
	}
	mavlink_message_t mavlink_msg;
	mavlink_heartbeat_t mavlink_heart_beat;
	memset(&mavlink_heart_beat,0,sizeof(mavlink_heartbeat_t));
	mavlink_heart_beat.type=MAV_TYPE_QUADROTOR;
	mavlink_heart_beat.system_status=MAV_STATE_STANDBY; //standby
	mavlink_msg_heartbeat_encode(SYSTEM_ID,COMPONENT_ID,&mavlink_msg,&mavlink_heart_beat);
	if (_mavlink_serial->write_data((const uint8_t*)(&mavlink_msg),sizeof(mavlink_message_t)) == 0){
		DERR("send heartbeat msg err.");
		return ;
	}
}
void MavlinkRouter::sendLocation(double lat, double lon, float relative_alt,float vx,float vy,float vz){
	if(_mavlink_serial == nullptr){
		DERR(" MAVlink serial is nullptr.");
		return;
	}
	mavlink_message_t mavlink_msg;
	mavlink_global_position_int_t mvalink_global_pos;
	memset(&mvalink_global_pos,0,sizeof(mavlink_global_position_int_t));
	mvalink_global_pos.lat=(int32_t)lat*1E7;//deg*1e7
	mvalink_global_pos.lon=(int32_t)lon*1E7;//deg*1e7
	mvalink_global_pos.relative_alt=(int32_t)relative_alt*1E3;  // mm
	mvalink_global_pos.vx=vx*1E2;//cm
	mvalink_global_pos.vy=vy*1E2;
	mvalink_global_pos.vz=vz*1E2;
	mavlink_msg_global_position_int_encode(SYSTEM_ID,COMPONENT_ID,&mavlink_msg,&mvalink_global_pos);
	if (_mavlink_serial->write_data((const uint8_t*)(&mavlink_msg),sizeof(mavlink_message_t)) == 0){
		DERR("send global pos msg err");
		return ;
	}
}
void MavlinkRouter:: stopMAVlinkThread(){
	
	if(_mavlink_router_read_thread != nullptr){
		_mavlink_thread_need_exit=true;
		_mavlink_router_read_thread->join();
		delete _mavlink_router_read_thread;
		_mavlink_router_read_thread=nullptr;
	}
	if (_mavlink_router_read_thread_mutex != nullptr){
		delete _mavlink_router_read_thread_mutex;
		_mavlink_router_read_thread_mutex=nullptr;
	}
	//stop this ev 
	ev_timer_stop(_mvalink_router_ev_loop,&_1hz_loop);
	//last delete the mavlink serial
	if (_mavlink_serial != nullptr){
		delete _mavlink_serial;
		_mavlink_serial=nullptr;
	}
}
void MavlinkRouter::mavlinkRouterReadThread(){
	uint8_t c;
	mavlink_message_t mavlink_msg;
	mavlink_status_t mavlink_status;
	unsigned int byte_count=0;
	//read loop
	while(_mavlink_thread_need_exit){
		if(_mavlink_serial->read_one_data(&c) <= 0){
			usleep(50000); //sleep 50ms
			continue;
		}
		unsigned int  decode_state=mavlink_parse_char(MAVLINK_CHANNEL_0, c, &mavlink_msg, &mavlink_status);
		if(decode_state==0){
			byte_count++;
			// 500 byte with no mavlink message
			if(byte_count>500){
				//TODO: add process	with parse status 
				DWAR("Exceed 500 byte with no MAVlink message.");
				byte_count=0;
				usleep(10000);		
			}
		}
		if(decode_state==1){
			handle_mavlink_message(mavlink_msg);
			usleep(10000); // complete a pack sleep 10 ms
		}
	}
}
void MavlinkRouter::handle_mavlink_message(const mavlink_message_t &mavlink_msg){
	// juast handle the linked GCS	
	if(mavlink_msg.sysid != GCS_SYSTEM_ID){
		return ;
	}
	switch(mavlink_msg.msgid){
		case MAVLINK_MSG_ID_HEARTBEAT:
			_handleHeartbeat(mavlink_msg);
			break;
		case MAVLINK_MSG_ID_COMMAND_LONG:
			_handleCommandLong(mavlink_msg);
			break;
		default:
			return;
	}		
}
void MavlinkRouter::_handleHeartbeat(const mavlink_message_t &mavlink_msg){
	
}
void MavlinkRouter::_handleCommandLong(const mavlink_message_t &mavlink_msg){
	
}
void MavlinkRouter::_one_hz_timer_callback(struct ev_loop* ev_loop, ev_timer *timer,int event){
	//send heart beat
	sendHeartbeat();
}
