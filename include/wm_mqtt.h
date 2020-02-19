#ifndef INCLUDE_WM_MQTT_H_
#define INCLUDE_WM_MQTT_H_

#include "mqtt.h"

void ICACHE_FLASH_ATTR mqtt_init();
void ICACHE_FLASH_ATTR mqttReInit();
void ICACHE_FLASH_ATTR mqttConnect();
void ICACHE_FLASH_ATTR mqttDisconnect();
bool ICACHE_FLASH_ATTR mqttPublish(char *topic, char*buff, uint16_t len);

#endif /* INCLUDE_WM_MQTT_H_ */
