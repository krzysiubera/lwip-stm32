#include <MQTTInterface.h>
#include <stdint.h>
#include <string.h>
#include "lwip/api.h"
#include "lwip/sockets.h"

uint32_t mili_timer;

char TimerIsExpired(Timer* timer)
{
	long left = timer->end_time - mili_timer;
	return (left < 0);
}

void TimerCountdownMS(Timer* timer, unsigned int timeout)
{
	timer->end_time = mili_timer + timeout;
}

void TimerCountdown(Timer* timer, unsigned int timeout)
{
	timer->end_time = mili_timer + (timeout * 1000);
}

int TimerLeftMS(Timer *timer)
{
	long left = timer->end_time - mili_timer;
	return (left < 0) ? 0 : left;
}

void TimerInit(Timer *timer)
{
	timer->end_time = 0;
}

int net_read(Network* network, unsigned char* buffer, int len, int timeout_ms)
{
	int available;

	if (lwip_ioctl(network->socket, FIONREAD, &available) < 0)
		return -1;

	if (available > 0)
		return lwip_recv(network->socket, buffer, len, 0);

	return 0;
}

int net_write(Network* network, unsigned char* buffer, int len, int timeout_ms)
{
	return lwip_send(network->socket, buffer, len, 0);
}

void NewNetwork(Network* network)
{
	network->socket = 0;
	network->mqttread = net_read;
	network->mqttwrite = net_write;
}

int ConnectNetwork(Network* network, char* ip, int port)
{
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_len = sizeof(addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip);

	network->socket = lwip_socket(AF_INET, SOCK_STREAM, 0);
	if (network->socket < 0)
	{
		network->socket = 0;
		return -1;
	}

	int ret_code = lwip_connect(network->socket, (struct sockaddr*) &addr, sizeof(addr));
	if (ret_code < 0)
	{
		lwip_close(network->socket);
		return -1;
	}

	return 0;
}
