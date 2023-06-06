#ifndef MQTT_DEFS_H
#define MQTT_DEFS_H

typedef struct Timer Timer;
struct Timer
{
	unsigned long systick_period;
	unsigned long end_time;
};

typedef struct Network Network;
struct Network
{
	int socket;
	int (*mqttread)(Network*, unsigned char*, int, int);
	int (*mqttwrite)(Network*, unsigned char*, int, int);
};

#endif
