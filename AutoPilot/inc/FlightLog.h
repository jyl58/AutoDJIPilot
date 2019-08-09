/*
* @file FlightLog.h
* @author: JYL
* @email:jiyingliang369@126.com
* @date: 2019.06.10
*
*/
#pragma once
#include <fstream>
#include <thread>
#include <mutex>
#include <string>
#ifdef OFFLINE_DEBUG
	#define LOG_PATH "/var/log/AutoDjiPilot/"
#else
	#define LOG_PATH "/mnt/dietpi_userdata/AutoDjiPilotLog/"
#endif
class FlightLog{
public:
		FlightLog(){};
		FlightLog(const FlightLog&)=delete;
		FlightLog& operator=(const FlightLog&)=delete;
		~FlightLog();
		static bool FlightLogInit();
		static void FlightLogStop();
		static void writeLogBuffer(const std::string& log_context);
		static void writeLogBufferWithLabel(const std::string& log_context);
private:
		static void writeLogThread();
		static std::string _log_buff;
		
		/*thread control*/
		static std::thread* _flight_log_thread;
		static std::mutex* _write_log_mutex;
		static bool _thread_need_exit;
};

