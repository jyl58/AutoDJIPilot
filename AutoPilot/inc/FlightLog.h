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
#define LOG_PATH "/"
class FlightLog{
public:
		FlightLog();
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

