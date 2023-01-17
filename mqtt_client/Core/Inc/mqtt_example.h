/*
 * mqtt_example.h
 *
 *  Created on: 17 Jan 2023
 *      Author: krzysiu
 */

#ifndef INC_MQTT_EXAMPLE_H_
#define INC_MQTT_EXAMPLE_H_

#include "mqtt.h"

void example_do_connect(mqtt_client_t *client);
void example_publish(mqtt_client_t* client, void* arg);

#endif /* INC_MQTT_EXAMPLE_H_ */
