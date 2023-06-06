#ifndef MQTT_INTERFACE_H
#define MQTT_INTERFACE_H

#include "MQTTDefs.h"

void InitTimer(Timer*);
char TimerIsExpired(Timer*);
void TimerCountdownMS(Timer*, unsigned int);
void TimerCountdown(Timer*, unsigned int);
int TimerLeftMS(Timer*);

int net_read(Network*, unsigned char*, int, int);
int net_write(Network*, unsigned char*, int, int);
void net_disconnect(Network*);
void NewNetwork(Network*);
int ConnectNetwork(Network*, char*, int);


#endif
