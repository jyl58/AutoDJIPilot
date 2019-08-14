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
#include "DjiCameraZoomType.h"
#include "dji_vehicle_callback.hpp"

#define MAX_SPEED_FACTOR 5.0
#define MIN_SPEED_FACTOR 0.3
#define BREAK_BOUNDARY 5.0
#define MAX_PKG_COUNT 5

using namespace DJI::OSDK;
using namespace DJI::OSDK::Telemetry;
typedef void (*function)(void);
struct PERIOD_CALL_LOOP{
	function _func;
	int      _call_freq;
};
class FlightCore{
public:
		FlightCore();
		FlightCore(const FlightCore&)=delete;
		FlightCore& operator=(const FlightCore&)=delete;
		virtual ~FlightCore();
		bool 	flightCoreInit(std::shared_ptr<DJI::OSDK::Vehicle> vehicle);
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
		virtual bool 	djiTakeoff(){return false;}
		virtual bool 	djiMoveByPosOffset(float x_offset_Desired,float y_offset_Desired ,
										   float z_offset_Desired, float yaw_Desired,
										   float pos_threshold_in_m=0.2,float yaw_threshold_in_deg=1.0)
										   {return false;}
		virtual bool 	djiMoveByGPS(double target_lat_deg,double target_lon_deg) {return false;}
		virtual bool 	djiMoveX_YByOffset(float target_x_m,float target_y_m,float pos_threshold_in_m=0.5){return false;}
		virtual bool	djiMoveByBearingAndDistance(float bearing,float distance){return false;}
		virtual bool 	djiMoveZByOffset(float target_alt_m,float vertical_threshold_in_m=0.5){return false;}
		virtual bool 	djiMoveZToTarget(float target_alt_m){return false;}
		virtual bool 	djiMoveByVelocity(float vx,float vy,float vz){return false;}
		virtual bool 	djiHover(){return false;}
		virtual bool 	djiTurnHead(float target_head_deg,float yaw_threshold_in_deg=1.0){return false;}
		virtual bool 	djiLanding(){return false;}
		virtual bool 	djiGoHome(){return false;}
		bool 	djiArmMotor();
		bool 	djiDisarmMotor();
		bool	djiShootPhoto();
		bool	djiVideoStart();
		bool    djiVideoStop();
		bool	djiSetGimbalAngle(float roll_deg,float pitch_deg,float yaw_deg);
		bool	djiSetGImbalSpeed(float roll_rate,float pitch_rate,float yaw_rate);
		bool	djiCameraZoomByPos(uint16_t times);
		bool	djiCameraZoomBySpeed(int16_t speed);
		bool	djiCameraZoomBystep(int16_t times);
		bool	djiCameraZoom(uint16_t mode, int16_t value);
		bool	djiCameraZoom(const camera_zoom_data_type_t *zoom);
		static bool	djiGetControlAuthority();
		static bool	djiReleaseControlAuthority();
		
protected:
		void readVehicleStatusThread();
		void periodFunctionScheduler();
		static void sendVehicleLocation();
		static void sendBatteryInfo();
		static void logLocation();
		static void PKGIndex_0_Callback(Vehicle* vehicle,RecvContainer recvFrame,UserData usrData);
		static void PKGIndex_1_Callback(Vehicle* vehicle,RecvContainer recvFrame,UserData usrData);
		static void PKGIndex_2_Callback(Vehicle* vehicle,RecvContainer recvFrame,UserData usrData);
		static void PKGIndex_3_Callback(Vehicle* vehicle,RecvContainer recvFrame,UserData usrData);
		static void PKGIndex_4_Callback(Vehicle* vehicle,RecvContainer recvFrame,UserData usrData);
		static void checkRCInterruptLuaRun();
		/*pointer to Vehicle*/
		static std::shared_ptr<DJI::OSDK::Vehicle> _vehicle;
		
		static const struct PERIOD_CALL_LOOP _period_call_tabe[];
		double* _last_run_time;
		
		static bool _vehicle_rtk_avilable;
		static TypeMap<TOPIC_STATUS_FLIGHT>::type 		_flightStatus;
		static TypeMap<TOPIC_STATUS_DISPLAYMODE>::type	_display_mode;
		static TypeMap<TOPIC_GPS_FUSED>::type 			_current_lat_lon;
		static TypeMap<TOPIC_GPS_DETAILS>::type 		_gps_details;
		static TypeMap<TOPIC_GPS_SIGNAL_LEVEL>::type 	_gps_signal_level;
		static TypeMap<TOPIC_HEIGHT_FUSION>::type 		_height_fusioned;
		static TypeMap<TOPIC_RC_FULL_RAW_DATA>::type 	_rc_full_raw_data;
		static TypeMap<TOPIC_RC>::type 					_rc_data;
		static TypeMap<TOPIC_RC_WITH_FLAG_DATA>::type 	_rc_witch_flag;
		static TypeMap<TOPIC_VELOCITY>::type 			_velocity;
		static TypeMap<TOPIC_QUATERNION>::type 			_quaternion;
		static TypeMap<TOPIC_BATTERY_INFO>::type 		_battery_info;
		static TypeMap<TOPIC_RTK_POSITION>::type 		_rtk_pos;
		static TypeMap<TOPIC_RTK_POSITION_INFO>::type 	_rtk_pos_info;
		static TypeMap<TOPIC_RTK_VELOCITY>::type 		_rtk_velocity;
		static TypeMap<TOPIC_RTK_YAW>::type				_rtk_yaw;
		static TypeMap<TOPIC_RTK_YAW_INFO>::type		_rtk_yaw_info;
		static TypeMap<TOPIC_GIMBAL_ANGLES>::type		_gimbal_angle;
		static TypeMap<TOPIC_GIMBAL_STATUS>::type		_gimbal_status;
		static TypeMap<TOPIC_GIMBAL_CONTROL_MODE>::type	_gimbal_mode;

		/*thread control */
		std::thread* _dji_FC_link_thread=nullptr;
		bool _thread_need_exit;
		static std::mutex _vehicle_data_mutex;
};
