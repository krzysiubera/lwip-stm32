/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "MQTTClient.h"
#include "MQTTInterface.h"
#include "lwip.h"
#include "lwip/api.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BROKER_IP "192.168.1.2"
#define MQTT_PORT 1883
#define MQTT_BUFSIZE 200
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
osThreadId mqtt_client_pub_task_handle;

Network network;
MQTTClient mqtt_client;

uint8_t send_buffer[MQTT_BUFSIZE];
uint8_t recv_buffer[MQTT_BUFSIZE];
uint8_t msg_buffer[MQTT_BUFSIZE];

uint32_t current_time = 0;
uint32_t previous_time = 0;

/* USER CODE END Variables */
osThreadId defaultTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void mqtt_client_pub_task(void const* argument);
int mqtt_connect_broker(void);
void mqtt_message_arrived(MessageData* msg);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);

extern void MX_LWIP_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 512);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* init code for LWIP */
  MX_LWIP_Init();
  /* USER CODE BEGIN StartDefaultTask */

  osThreadDef(mqttClientPubTask, mqtt_client_pub_task, osPriorityNormal, 0, configMINIMAL_STACK_SIZE);
  mqtt_client_pub_task_handle = osThreadCreate(osThread(mqttClientPubTask), NULL);

  /* Infinite loop */
  for(;;)
  {
    osDelay(500);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

void mqtt_client_pub_task(void const* argument)
{
	NewNetwork(&network);

	int ret;
	ret = ConnectNetwork(&network, BROKER_IP, MQTT_PORT);
	if (ret != MQTT_SUCCESS)
	{
		printf("ConnectNetwork failed.");
		while(1);
	}

	MQTTClientInit(&mqtt_client, &network, 1000, send_buffer, sizeof(send_buffer), recv_buffer, sizeof(recv_buffer));

	MQTTPacket_connectData connect_data = MQTTPacket_connectData_initializer;
	connect_data.willFlag = 1;
	connect_data.will.topicName.cstring = "info/device";
	connect_data.will.message.cstring = "I was disconnected";
	connect_data.will.qos = 0;
	connect_data.will.retained = 0;
	connect_data.MQTTVersion = 4;
	connect_data.clientID.cstring = "stm krzysiu";
	connect_data.username.cstring = "login";
	connect_data.password.cstring = "pass";
	connect_data.keepAliveInterval = 300;
	connect_data.cleansession = 1;

	ret = MQTTConnect(&mqtt_client, &connect_data);
	if (ret != MQTT_SUCCESS)
	{
		printf("MQTTConnect failed. \n");
		while(1);
	}

	char* str_qos_0 = "qos 0 msg";
	MQTTMessage msg_qos_0 = { .payload=(void*)(str_qos_0), .payloadlen=strlen(str_qos_0), .qos=0, .retained=0 };
	MQTTPublish(&mqtt_client, "sensor/temp", &msg_qos_0);

	char* str_qos_1 = "qos 1 msg";
	MQTTMessage msg_qos_1 = { .payload=(void*)(str_qos_1), .payloadlen=strlen(str_qos_1), .qos=1, .retained=0 };
	MQTTPublish(&mqtt_client, "sensor/temp", &msg_qos_1);

	char* str_qos_2 = "qos 2 msg";
	MQTTMessage msg_qos_2 = { .payload=(void*)(str_qos_2), .payloadlen=strlen(str_qos_2), .qos=2, .retained=0 };
	MQTTPublish(&mqtt_client, "sensor/temp", &msg_qos_2);


	MQTTSubscribe(&mqtt_client, "drive/voltage", QOS0, mqtt_message_arrived);
	MQTTSubscribe(&mqtt_client, "drive/current", QOS1, mqtt_message_arrived);
	MQTTSubscribe(&mqtt_client, "drive/power", QOS2, mqtt_message_arrived);

	char* str_new = "check if ok payload";
	MQTTMessage msg_new = { .payload=(void*)(str_new), .payloadlen=strlen(str_new), .qos=2, .retained=0 };
	MQTTPublish(&mqtt_client, "sensor/temp", &msg_new);

	while(1)
	{
		MQTTYield(&mqtt_client,  1000);
	}
}

void mqtt_message_arrived(MessageData* msg)
{
	MQTTMessage* message = msg->message;
	memcpy(msg_buffer, message->payload, message->payloadlen);

	printf("MQTT MSG[%d]:%s\n", (int)message->payloadlen, msg_buffer);
}




/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
