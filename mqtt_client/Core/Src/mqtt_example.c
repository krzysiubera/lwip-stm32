/*
 * mqtt_example.c
 *
 *  Created on: 17 Jan 2023
 *      Author: krzysiu
 */

#include <string.h>
#include "mqtt.h"
#include "mqtt_example.h"

/* callbacks */
static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status);
static void mqtt_sub_request_cb(void *arg, err_t result);
void mqtt_pub_request_cb(void* arg, err_t result);


/* Example on how to make MQTT connection to broker */
void example_do_connect(mqtt_client_t *client)
{
	  struct mqtt_connect_client_info_t ci;
	  err_t err;

	  /* Setup an empty client info structure */
	  memset(&ci, 0, sizeof(ci));

	  /* Minimal amount of information required is client identifier, so set it here */
	  ci.client_id = "lwip_test";
	  ci.client_user = NULL;
	  ci.client_pass = NULL;
	  ci.keep_alive = 120;

	  /* Initiate client and connect to server, if this fails immediately an error code is returned
		 otherwise mqtt_connection_cb will be called with connection result after attempting
		 to establish a connection with the server.
		 For now MQTT version 3.1.1 is always used */
	  ip_addr_t mqtt_server_ip;
	  ipaddr_aton("192.168.1.2", &mqtt_server_ip);
	  err = mqtt_client_connect(client, &mqtt_server_ip, MQTT_PORT, mqtt_connection_cb, 0, &ci);

	  /* For now just print the result code */
	  printf("mqtt_connect return %d\n", err);
}

/* Example on how to publish data */
void example_publish(mqtt_client_t* client, void* arg)
{
	const char* pub_payload = "this is my payload from stm";
	u8_t qos = 2;
	u8_t retain = 0;
	err_t err = mqtt_publish(client, "pub_topic", pub_payload, strlen(pub_payload), qos, retain,
							mqtt_pub_request_cb, arg);

	printf("mqtt_publish return: %d\n", err);

}

/* Called when client has connected to the server attempt by:
 * calling mqtt_connect() or when connection is closed by server or an error
 */
static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
	if (status != MQTT_CONNECT_ACCEPTED)
	{
		printf("mqtt_connection_cb: Disconnected, reason: %d\n", status);
		example_do_connect(client);
	}
	else
	{
		/* Subscribe to a topic named "sub_topic" with QoS level 0, call mqtt_sub_request_cb with result */
		err_t err = mqtt_subscribe(client, "sub_topic", 0, mqtt_sub_request_cb, arg);
		printf("mqtt_subscribe return: %d\n", err);
	}
}

/* Called when a subscribe request has completed */
static void mqtt_sub_request_cb(void *arg, err_t result)
{
	printf("Subscribe result: %d\n", result);
}

/* Called when a publish request has completed */
void mqtt_pub_request_cb(void* arg, err_t result)
{
	if (result != ERR_OK)
	{
		printf("Publish result: %d\n", result);
	}
}
