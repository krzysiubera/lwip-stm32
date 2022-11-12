/*
 * tcp_echo.h
 *
 *  Created on: 8 Nov 2022
 *      Author: krzysiu
 */

#ifndef INC_TCP_ECHO_H_
#define INC_TCP_ECHO_H_

#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"

#define  ECHO_SERVER_LISTEN_PORT	7

err_t tcp_echoserver_init(void);

#endif /* INC_TCP_ECHO_H_ */
