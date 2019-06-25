/*
* @file FlightCore.h
* @author: JYL
* @email:jiyingliang369@126.com
* @date: 2019.06.10
*
*/
#pragma once

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <thread>
#include <mutex>
#include "dji_vehicle.hpp"
#include "dji_telemetry.hpp"
#include "dji_status.hpp"

#define MAX_SPEED_FACTOR 5.0
#define BREAK_BOUNDARY 5.0
#define MAX_PKG_COUNT 5

using namespace DJI::OSDK;
using namespace DJI::OSDK::Telemetry;
class FlightCore{
public:
		FlightCore(DJI::OSDK::Vehicle* vehicle);
		~FlightCore();
		bool 	flightCoreInit();
		void 	exitDjiThread();
		/*get vehicle flight data*/
		void 	getFlightStatus(TypeMap<TOPIC_STATUS_FLIGHT>::type *flightStatus);
		void 	getFlightBatteryInfo(TypeMap<TOPIC_BATTERY_INFO>::type* battery_info);
		void 	getVehicleGPS(TypeMap<TOPIC_GPS_FUSED>::type 		*lat_lon);
		void 	getVehicleAltitude(TypeMap<TOPIC_HEIGHT_FUSION>::type 	*altitude);
		void 	getVehicleVelocity(TypeMap<TOPIC_VELOCITY>::type 	*velocity);
		void 	getVehicleQuaternion(TypeMap<TOPIC_QUATERNION>::type *quaternion);
		float 	getVehicleBearing();
		void 	getVehicleDisplay(TypeMap<TOPIC_STATUS_DISPLAYMODE>::type*	display_mode);
		void 	getVehicleGpsDetails(TypeMap<TOPIC_GPS_DETAILS>::type* gps_details);
		void 	getVehicleGpsSignalLevel(TypeMap<TOPIC_GPS_SIGNAL_LEVEL>::type* gps_signal);
		void	getGimbalAngle(TypeMap<TOPIC_GIMBAL_ANGLES>::type	*gimbal_angle);
		void	getGimbalStatus(TypeMap<TOPIC_GIMBAL_STATUS>::type* gimbal_status);
		uint8_t	getGimbalMode(){return _gimbal_mode;}
		/* control vehicle flight*/
		bool 	djiTakeoff();
		bool 	djiMoveByPosOffset(float x_offset_Desired,float y_offset_Desired ,float z_offset_Desired, float yaw_Desired,float pos_threshold_in_m=0.2,float yaw_threshold_in_deg=1.0);
		bool 	djiMoveByGPS(double target_lat_deg,double target_lon_deg);
		bool 	djiMoveX_YByOffset(float target_x_m,float target_y_m,float pos_threshold_in_m=0.5);
		bool	djiMoveByBearingAndDistance(float bearing,float distance);
		bool 	djiMoveZByOffset(float target_alt_m,float vertical_threshold_in_m=0.5);
		bool 	djiMoveZToTarget(float target_alt_m);
		bool 	djiMoveByVelocity(float vx,float vy,float vz);
		bool 	djiTurnHead(float target_head_deg,float yaw_threshold_in_deg=1.0);
		bool 	djiLanding();
		bool 	djiGoHome();
		bool 	djiArmMotor();
		bool 	djiDisarmMotor();
		bool	djiShootPhoto();
		bool	djiVideoStart();
		bool    djiVideoStop();
		bool	djiCameraZoom();
		bool	djiSetGimbalAngle(float roll_deg,float pitch_deg,float yaw_deg);
		bool	djiSetGImbalSpeed(float roll_rate,float pitch_rate,float yaw_rate);
		static void	djiNeedBreakAutoControl(bool need_break){_auto_running_need_break=need_break;}
		
		
private:
		void readVehicleStatusThread();
		/*pointer to Vehicle*/
		DJI::OSDK::Vehicle* _vehicle=nullptr;
		bool _vehicle_rtk_avilable;
		TypeMap<TOPIC_STATUS_FLIGHT>::type 		_flightStatus;
		TypeMap<TOPIC_STATUS_DISPLAYMODE>::type	_display_mode;
		TypeMap<TOPIC_GPS_FUSED>::type 			_current_lat_lon;
		TypeMap<TOPIC_GPS_DETAILS>::type 		_gps_details;
		TypeMap<TOPIC_GPS_SIGNAL_LEVEL>::type 	_gps_signal_level;
		TypeMap<TOPIC_HEIGHT_FUSION>::type 	_height_fusioned;
		TypeMap<TOPIC_RC_FULL_RAW_DATA>::type 	_rc_full_raw_data;
		TypeMap<TOPIC_RC>::type 				_rc_data;
		TypeMap<TOPIC_VELOCITY>::type 			_velocity;
		TypeMap<TOPIC_QUATERNION>::type 		_quaternion;
		TypeMap<TOPIC_BATTERY_INFO>::type 		_battery_info;
		TypeMap<TOPIC_RTK_POSITION>::type 		_rtk_pos;
		TypeMap<TOPIC_RTK_POSITION_INFO>::type 	_rtk_pos_info;
		TypeMap<TOPIC_RTK_VELOCITY>::type 		_rtk_velocity;
		TypeMap<TOPIC_RTK_YAW>::type			_rtk_yaw;
		TypeMap<TOPIC_RTK_YAW_INFO>::type		_rtk_yaw_info;
		TypeMap<TOPIC_GIMBAL_ANGLES>::type		_gimbal_angle;
		TypeMap<TOPIC_GIMBAL_STATUS>::type		_gimbal_status;
		TypeMap<TOPIC_GIMBAL_CONTROL_MODE>::type		_gimbal_mode;

		/*thread control */
		std::thread* _dji_FC_link_thread=nullptr;
		bool _thread_need_exit;
		std::mutex *_vehicle_data_mutex=nullptr;

		/*break auto control running flag*/
		static bool _auto_running_need_break;
};
