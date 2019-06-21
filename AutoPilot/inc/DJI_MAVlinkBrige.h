/*
* @file DJI_MAVlinkBrige.h
* @author: JYL
* @email:jiyingliang369@126.com
* @date: 2019.06.10
*
*/
#pragma once
#include "dji_vehicle.hpp"
#include "dji_vehicle_callback.hpp"
/*callback for mavlink route the global pos  */
void GlobalPosCallback(DJI::OSDK::Vehicle* vehicle,RecvContainer recvFrame,UserData usrData);
/*callback for rc input for payload */
void RCCallback(DJI::OSDK::Vehicle* vehicle,RecvContainer recvFrame,UserData usrData);
