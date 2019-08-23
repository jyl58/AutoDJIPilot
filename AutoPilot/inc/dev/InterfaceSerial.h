/*
 * File:   InterfaceSerial.hpp
 * Author: jiyingliang
 * Email: jiyingliang369@126.com
 * Date:  2019.6.4
 */

#ifndef INTERFACE_UART_HPP
#define	INTERFACE_UART_HPP

#define TRUE 1
#define FALSE 0

#include <stdlib.h>
#include <inttypes.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/time.h>
#include <mutex>

typedef unsigned char uint8_t;

class InterfaceSerial {

public:
    InterfaceSerial(const char *portname, int speed=115200);
    virtual ~InterfaceSerial();

    int SetupSerial(int fdes,int baud,int databits,int stopbits,int parity);
    void flush_buffer(void);
    int read_data(uint8_t *message);
	int read_one_data(uint8_t *message);
	double tic();
    int write_data(const uint8_t *data_out,int byte_count);
	int write_data(uint8_t data);

private:

    void set_blocking (int fd, int should_block);
    int set_interface_attribs (int fd, int speed, int parity);
	/* serial dev descriptor */    
	int _fd;
	std::mutex _fd_mutex;
};
#endif	/* INTERFACE_UART_HPP */
