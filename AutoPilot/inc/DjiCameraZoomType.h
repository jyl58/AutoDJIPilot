/*
* @file FlightCore.h
* @author: JYL
* @email:jiyingliang369@126.com
* @date: 2019.06.26
*
*/
#pragma once 
// Camera zoom data struct 
#pragma pack(1)
typedef struct zoom_config_t
{
  uint8_t digital_zoom_mode 	: 2;	/* 0:step 1:position 2:continuous */
  uint8_t digital_reserve 	: 1;	/* reserve */
  uint8_t digital_zoom_enable	: 1;	/* 0:not_ctrl 1:ctrl */
  uint8_t optical_zoom_mode  	: 2;	/* 0:step 1:position 2:continuous */
  uint8_t optical_reserve		: 1;	/* reserve */
  uint8_t optical_zoom_enable	: 1;	/* 0:not_ctrl 1:ctrl */
} zoom_config_t;

typedef union zoom_param_t
{
  struct
  {
    uint16_t zoom_cont_speed        : 8;	/* continuous speed 0~100 */
    uint16_t zoom_cont_direction    : 1;
    uint16_t zoom_cont_reserve      : 7;
  }cont_param;
  struct
  {
    uint16_t zoom_step_level		: 8;	/* level time * 100 = 1 times */
    uint16_t zoom_step_direction    : 1;
    uint16_t zoom_step_reserve      : 7;
  }step_param;
  struct
  {
    uint16_t zoom_pos_level;		        /* 180 = 1.8times */
  }pos_param;
} zoom_param_t;

typedef struct camera_zoom_data_type
{
  uint8_t func_index;
  uint8_t cam_index;
  zoom_config_t zoom_config;
  zoom_param_t optical_zoom_param;
  zoom_param_t digital_zoom_param;
}camera_zoom_data_type_t;
#pragma pack()
