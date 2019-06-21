/*
* @file geo.cpp
* @author: JYL
* @email:jiyingliang369@126.com
* @date: 2019.06.10
*
*/
#include <cmath>
#include "geo.h"

float _get_bearing_to_next_waypoint(double lat_now, double lon_now, double lat_next, double lon_next)
{
    double lat_now_rad = DEG2RAD*lat_now;
    double lon_now_rad = DEG2RAD*lon_now;
    double lat_next_rad = DEG2RAD*lat_next;
    double lon_next_rad = DEG2RAD*lon_next;

    double d_lon = lon_next_rad - lon_now_rad;

    // conscious mix of double and float trig function to maximize speed and efficiency
    float theta = atan2f(sin(d_lon) * cos(lat_next_rad) ,
                 cos(lat_now_rad) * sin(lat_next_rad) - sin(lat_now_rad) * cos(lat_next_rad) * cos(d_lon));

    theta = _wrap_pi(theta);

    return theta;
}
float _get_distance_to_next_waypoint(double lat_now, double lon_now, double lat_next, double lon_next)
{
     double lat_now_rad = DEG2RAD*lat_now;
     double lon_now_rad = DEG2RAD*lon_now;
     double lat_next_rad = DEG2RAD*lat_next;
     double lon_next_rad = DEG2RAD*lon_next;

     double d_lat = lat_next_rad - lat_now_rad;
     double d_lon = lon_next_rad - lon_now_rad;

     double a = sin(d_lat / (double)2.0) * sin(d_lat / (double)2.0) + sin(d_lon / (double)2.0) * sin(d_lon /(double)2.0) * cos(lat_now_rad) * cos(lat_next_rad);
     double c = (double)2.0 * atan2(sqrt(a), sqrt((double)1.0 - a));

     return CONSTANTS_RADIUS_OF_EARTH * c;
}
void  _waypoint_from_heading_and_distance(double lat_start, double lon_start, float bearing, float distance,double *lat_target, double *lon_target)
{
    bearing = _wrap_2pi(bearing);
    double radius_ratio = (double)(fabs(distance) / CONSTANTS_RADIUS_OF_EARTH);

    double lat_start_rad = DEG2RAD*lat_start;
    double lon_start_rad = DEG2RAD*lon_start;

    *lat_target = asin(sin(lat_start_rad) * cos(radius_ratio) + cos(lat_start_rad) * sin(radius_ratio) * cos((double)bearing));
    *lon_target = lon_start_rad + atan2(sin((double)bearing) * sin(radius_ratio) * cos(lat_start_rad), cos(radius_ratio) - sin(lat_start_rad) * sin(*lat_target));

    *lat_target = RAD2DEG*(*lat_target);
    *lon_target = RAD2DEG*(*lon_target);
}
void _get_vector_to_next_waypoint(double lat_now, double lon_now, double lat_next, double lon_next, float *v_n,float *v_e)
{
    double lat_now_rad = DEG2RAD*lat_now;
    double lon_now_rad = DEG2RAD*lon_now;
    double lat_next_rad = DEG2RAD*lat_next;
    double lon_next_rad = DEG2RAD*lon_next ;

    double d_lon = lon_next_rad - lon_now_rad;

    /* conscious mix of double and float trig function to maximize speed and efficiency */
    *v_n = CONSTANTS_RADIUS_OF_EARTH * (cos(lat_now_rad) * sin(lat_next_rad) - sin(lat_now_rad) * cos(lat_next_rad) * cos(d_lon));
    *v_e = CONSTANTS_RADIUS_OF_EARTH * sin(d_lon) * cos(lat_next_rad);
}
void _get_vector_to_next_waypoint_fast(double lat_now, double lon_now, double lat_next, double lon_next, float *v_n,float *v_e)
{
    double lat_now_rad = DEG2RAD*lat_now;
    double lon_now_rad = DEG2RAD*lon_now;
    double lat_next_rad = DEG2RAD*lat_next;
    double lon_next_rad = DEG2RAD*lon_next;

    double d_lat = lat_next_rad - lat_now_rad;
    double d_lon = lon_next_rad - lon_now_rad;

    /* conscious mix of double and float trig function to maximize speed and efficiency */
    *v_n = CONSTANTS_RADIUS_OF_EARTH * d_lat;
    *v_e = CONSTANTS_RADIUS_OF_EARTH * d_lon* cos(lat_now_rad);
}
void _add_vector_to_global_position(double lat_now, double lon_now, float v_n, float v_e, double *lat_res,double *lon_res)
{
    double lat_now_rad = DEG2RAD*lat_now;
    double lon_now_rad = DEG2RAD*lon_now;

    *lat_res =  RAD2DEG*(lat_now_rad + (double)v_n / CONSTANTS_RADIUS_OF_EARTH);
    *lon_res =  RAD2DEG*(lon_now_rad + (double)v_e / (CONSTANTS_RADIUS_OF_EARTH * cos(lat_now_rad)));
}
double _wrap_pi(float bearing)
{
    double res = _wrap_2pi(bearing);
    if (res > M_PI) {
       res -= 2*M_PI;
    }
    return res;
}
double _wrap_2pi(float bearing)
{
    double res = fmodf(bearing, (float)2*M_PI);
    if (res < 0) {
        res += 2*M_PI;
    }
    return res;
}
double _wrap_360(float angle, float unit_mod)
{
    const float ang_360 = 360.f * unit_mod;
    float res = fmodf(angle, ang_360);
    if (res < 0) {
        res += ang_360;
    }
    return res;
}

DJI::OSDK::Telemetry::Vector3f toEulerAngle(void* quaternionData){
	DJI::OSDK::Telemetry::Vector3f    ans;
	DJI::OSDK::Telemetry::Quaternion* quaternion = (DJI::OSDK::Telemetry::Quaternion*)quaternionData;

	double q2sqr= quaternion->q2 * quaternion->q2;
	double t0   = -2.0 * (q2sqr + quaternion->q3 * quaternion->q3) + 1.0;
	double t1   = +2.0 * (quaternion->q1 * quaternion->q2 + quaternion->q0 * quaternion->q3);
	double t2 	= -2.0 * (quaternion->q1 * quaternion->q3 - quaternion->q0 * quaternion->q2);
	double t3 	= +2.0 * (quaternion->q2 * quaternion->q3 + quaternion->q0 * quaternion->q1);
	double t4 	= -2.0 * (quaternion->q1 * quaternion->q1 + q2sqr) + 1.0;

	t2 = (t2 > 1.0) ? 1.0 : t2;
	t2 = (t2 < -1.0) ? -1.0 : t2;

	ans.x = asin(t2);
	ans.y = atan2(t3, t4);
	ans.z = atan2(t1, t0);

	return ans;
}

void localOffsetFromGPSOffset(DJI::OSDK::Telemetry::Vector3f& deltaNed,void* target,void* origin){
	DJI::OSDK::Telemetry::GPSFused*       subscriptionTarget;
	DJI::OSDK::Telemetry::GPSFused*       subscriptionOrigin;
	
	double  deltaLon=0;
	double  deltaLat=0;

	subscriptionTarget = (DJI::OSDK::Telemetry::GPSFused*)target;
	subscriptionOrigin = (DJI::OSDK::Telemetry::GPSFused*)origin;
	deltaLon   = subscriptionTarget->longitude - subscriptionOrigin->longitude;
	deltaLat   = subscriptionTarget->latitude - subscriptionOrigin->latitude;
	deltaNed.x = deltaLat * CONSTANTS_RADIUS_OF_EARTH;
	deltaNed.y = deltaLon * CONSTANTS_RADIUS_OF_EARTH* cos(subscriptionTarget->latitude);
	deltaNed.z = 0;//subscriptionTarget->altitude - subscriptionOrigin->altitude;
}
