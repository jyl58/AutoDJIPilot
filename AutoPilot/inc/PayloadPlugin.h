/*
* @file PayloadPlugin.h
* @author: JYL
* @email:jiyingliang369@126.com
* @date: 2019.06.12
*
*/
#pragma once
#include "PayloadBase.h"

/* return a new plugin instance,,need C for undef symbol */
extern "C" PayloadBase* PayloadInstance(void){
	return PayloadBase::getDeriveInstance();
}
