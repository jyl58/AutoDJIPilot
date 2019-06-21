/*
* @file geo.h
* @author: JYL
* @email:jiyingliang369@126.com
* @date: 2019.05.25
*
*/
#pragma once
#include "dji_telemetry.hpp"
#define CONSTANTS_RADIUS_OF_EARTH               (double)6378137.0			/* meters (m)	6378100	*/
#ifndef DEG2RAD 
#define DEG2RAD 0.01745329252
#endif
#ifndef RAD2DEG
#define RAD2DEG 57.295779513
#endif
/*calculate the bearing for two GPS point
* param lat_now: first GPS point latitude.   unit:  deg
* param lon_now: first GPS point longitude.  unit: deg
* param lat_next: scond GPS point latitude.  unit: deg 
* param lon_next: scond GPS point longitude. unit: deg
* return bearing value (-pi,pi).unit: rad
*/
float _get_bearing_to_next_waypoint(double lat_now, double lon_now, double lat_next, double lon_next);
/*calculate the distance for two GPS point
* param lat_now: first GPS point latitude.   unit:  deg
* param lon_now: first GPS point longitude.  unit: deg
* param lat_next: scond GPS point latitude.  unit: deg 
* param lon_next: scond GPS point longitude. unit: deg
* return: distance .unit: m
*/
float _get_distance_to_next_waypoint(double lat_now, double lon_now, double lat_next, double lon_next);
/*already know a GPS point and bearing and distance calculate next GPS point 
* param lat_start: first GPS point latitude.   unit:  deg
* param lon_start: first GPS point longitude.  unit: deg
* param bearing: the bearing for next GPS point. unit rad
* param distance: the distanc for next GPS point. unit: m
* param *lat_next: pointer for scond GPS point latitude.  unit: deg 
* param *lon_next: pointer for scond GPS point longitude. unit: deg
*/
void  _waypoint_from_heading_and_distance(double lat_start, double lon_start, float bearing, float distance,double *lat_target, double *lon_target);
/*
*	calculate the NE coordinate for next gps point reletive first gps poing 
*	param lat_now: first GPS point latitude.   unit:  deg
*	param lon_now: first GPS point longitude.  unit: deg
*	param lat_next: pointer for scond GPS point latitude.  unit: deg 
*	param lon_next: pointer for scond GPS point longitude. unit: deg
*	param *v_n: the x coordinate pointer. unit: m
*	param *v_e: the y coordinate pointer. unit: m
*/
void _get_vector_to_next_waypoint(double lat_now, double lon_now, double lat_next, double lon_next, float *v_n,float *v_e);
/*
*	fast calculate the NE coordinate for next gps point reletive first gps poing 
*	param lat_now: first GPS point latitude.   unit:  deg
*	param lon_now: first GPS point longitude.  unit: deg
*	param lat_next: pointer for scond GPS point latitude.  unit: deg 
*	param lon_next: pointer for scond GPS point longitude. unit: deg
*	param *v_n: the x coordinate pointer. unit: m
*	param *v_e: the y coordinate pointer. unit: m
*/
void _get_vector_to_next_waypoint_fast(double lat_now, double lon_now, double lat_next, double lon_next, float *v_n,float *v_e);
/*
*	already know a GPS point and NE coordinate calculate next GPS point 
*	param lat_start: first GPS point latitude.   unit:  deg
*	param lon_start: first GPS point longitude.  unit: deg
*	param v_n: x coordinate. unit: m
*	param v_e: y coordinate. unit: m
*	param *lat_next: pointer for scond GPS point latitude.  unit: deg 
*	param *lon_next: pointer for scond GPS point longitude. unit: deg
*/
void _add_vector_to_global_position(double lat_now, double lon_now, float v_n, float v_e, double *lat_res,double *lon_res);
/* 
*	convert a bearing to (-pi,pi]
	param bearing: the angle unit: rad
*/
double _wrap_pi(float bearing);
/* 
*	convert a bearing to (0,2pi]
*	param bearing: the angle unit: rad
*/
double _wrap_2pi(float bearing);
/* 
*	convert a bearing to (0,360]
*	param bearing: the angle unit: deg
*/
double _wrap_360(float angle, float unit_mod);

/*
*	convert quaternion to eular angle.unit: rad
*/
DJI::OSDK::Telemetry::Vector3f toEulerAngle(void* quaternionData);
/*
* get ne vector fast by two GPS point .unit: m
*/
void localOffsetFromGPSOffset(DJI::OSDK::Telemetry::Vector3f& local_offset,void* target,void* origin);	
