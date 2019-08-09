/*
* @file Matrice200.h
* @author: JYL
* @email:jiyingliang369@126.com
* @date: 2019.08.06
*
*/

#pragma once
#include "FlightCore.h"
class Matrice200 :public FlightCore
{
	public:
		Matrice200();
		~Matrice200(){};
		bool 	djiTakeoff() override;
		bool	djiLanding() override;
		bool	djiGoHome() override;
		bool 	djiHover()override;
		bool 	djiMoveByGPS(double target_lat_deg,double target_lon_deg)override;
		bool 	djiMoveByVelocity(float vx,float vy,float vz)override;
		bool 	djiMoveZByOffset(float target_alt_m,float vertical_threshold_in_m=0.5)override;
		bool 	djiMoveZToTarget(float target_alt_m)override;
		bool	djiMoveX_YByOffset(float target_x_m, float target_y_m, float pos_threshold_in_m=0.5)override;
		bool 	djiTurnHead(float target_head_deg,float yaw_threshold_in_deg=1.0)override;
		bool	djiMoveByBearingAndDistance(float bearing,float distance)override;
		bool	djiMoveByPosOffset(float x_offset_Desired,float y_offset_Desired,
								   float z_offset_Desired, float yaw_Desired,
								   float pos_threshold_in_m=0.2,float yaw_threshold_in_deg=1.0)override;

};
