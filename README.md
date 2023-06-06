This repository contains projects launched during my experiments with lwIP library - https://savannah.nongnu.org/projects/lwip/
They were run on STM32 Nucleo-144 development board with STM32F429ZI MCU - https://www.st.com/en/evaluation-tools/nucleo-f429zi.html

These programs are:
- tcp_echo_server - TCP server echoing back sent text using ST examples from this document: 
https://www.st.com/resource/en/user_manual/um1713-developing-applications-on-stm32cube-with-lwip-tcpip-stack-stmicroelectronics.pdf
It uses lwIP raw API.
- mqtt_client - MQTT client using lwIP implementation of this protocol. It uses lwIP raw API.
- mqtt_client_paho - MQTT client using Paho Embedded MQTT C Client library: https://www.eclipse.org/paho/index.php?page=clients/c/embedded/index.php and FreeRTOS.
It uses lwIP socket API.

Note: in all programs static IP address: 192.168.1.3 was set.
 