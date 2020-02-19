#include "esp8266.h"
#include "user_interface.h"
#include "spi_flash.h"

#include "global.h"
#include "wm_utils.h"
#include "wm_mqtt.h"

LOCAL MQTT_Client mqttClient;
LOCAL os_timer_t mqttDelClientTimer, mqttInitTimer;

void ICACHE_FLASH_ATTR mqttConnect() {
	MQTT_Connect(&mqttClient);
}

void ICACHE_FLASH_ATTR mqttDisconnect() {
	MQTT_Disconnect(&mqttClient);
}

bool ICACHE_FLASH_ATTR mqttPublish(char *topic, char *buff, uint16_t len) {
	bool ret = MQTT_Publish(&mqttClient, topic, buff, len, 0, 0);
	return ret;
}

void ICACHE_FLASH_ATTR mqttConnectedCb(uint32_t *args) {

	MQTT_Client* client = (MQTT_Client*) args;
	char *topic = (char*) os_malloc(
			strlen(wmConfig.mqttTopic) + SIZE_END_TOPIC);

	os_printf("MQTT: Connected\n");

	if (!topic) {
		INFO("Error allocation memory in %lu line in %s file\n", __LINE__, __FILE__);
		return;
	}

	os_sprintf(topic, "/%s/%s/%s", wmConfig.mqttTopic,
			getMacAddress(STATION_IF), END_TOPIC_HOT_IN);
	if (mqttFirstStart) {
		INFO("Full name in topic for hot water: %s\n", topic);
	}
	MQTT_Subscribe(client, topic, 0);

	os_sprintf(topic, "/%s/%s/%s", wmConfig.mqttTopic,
			getMacAddress(STATION_IF), END_TOPIC_COLD_IN);

	if (mqttFirstStart) {
		INFO("Full name in topic for cold water: %s\n", topic);
	}
	MQTT_Subscribe(client, topic, 0);

	if (mqttFirstStart) {
		os_sprintf(topic, "/%s/%s/%s", wmConfig.mqttTopic,
				getMacAddress(STATION_IF), END_TOPIC_HOT_OUT);
		INFO("Full name out topic for hot water: %s\n", topic);
		os_sprintf(topic, "/%s/%s/%s", wmConfig.mqttTopic,
				getMacAddress(STATION_IF), END_TOPIC_COLD_OUT);
		INFO("Full name out topic for cold water: %s\n", topic);
		mqttFirstStart = false;
	}

	os_free(topic);
	mqttConnected = true;

}

void ICACHE_FLASH_ATTR mqttDisconnectedCb(uint32_t *args) {

	mqttConnected = false;

}

void ICACHE_FLASH_ATTR mqttDataCb(uint32_t *args, const char* topic,
		uint32_t topic_len, const char *data, uint32_t data_len) {

	char *topicBuf = (char*) os_zalloc(topic_len + 1),*dataBuf = (char*) os_zalloc(data_len + 1);

	if (!topicBuf || !dataBuf) {
		INFO("Error allocation memory in %lu line in %s file\n", __LINE__, __FILE__);
	}
	char *p, snew[] = "NEW";
	uint32_t timeFromServer, waterFromServer;
	uint8_t pos;

	MQTT_Client* client = (MQTT_Client*) args;

	os_memcpy(topicBuf, topic, topic_len);
	topicBuf[topic_len] = 0;
	os_memcpy(dataBuf, data, data_len);
	dataBuf[data_len] = 0;

	INFO("%s ==> %s\n", dataBuf, topicBuf);

	pos = strcspn(dataBuf, " ");

	if (pos != strlen(dataBuf)) {

		dataBuf[pos] = 0;
		timeFromServer = strtoul(dataBuf, 0, 10);
		p = dataBuf + pos + 1;
		waterFromServer = strtoul(p, 0, 10);

		if (os_strstr(topicBuf, END_TOPIC_HOT_IN)) {
			if (os_strstr(p, snew)) {
				wmConfig.hotWater = waterFromServer;
				wmConfig.hotTime = timeFromServer;
				saveNewConfig = true;
			} else if (waterFromServer > wmConfig.hotWater) {
				wmConfig.hotWater = waterFromServer + wmConfig.litersPerPulse;
				wmConfig.hotTime = timeFromServer;
				saveNewConfig = true;
			}
		} else if (os_strstr(topicBuf, END_TOPIC_COLD_IN)) {
			if (os_strstr(p, snew)) {
				wmConfig.coldWater = waterFromServer;
				wmConfig.coldTime = timeFromServer;
				saveNewConfig = true;
			} else if (waterFromServer > wmConfig.coldWater) {
				wmConfig.coldWater = waterFromServer + wmConfig.litersPerPulse;
				wmConfig.coldTime = timeFromServer;
				saveNewConfig = true;
			}
		}
	}

	if (saveNewConfig) {
		saveNewConfig = false;
		saveConfig();
	}

	os_free(topicBuf);
	os_free(dataBuf);
}

void ICACHE_FLASH_ATTR mqtt_init() {

	os_printf("MQTT init\n");

	char client_id[32];

	mqttConnected = false;

	MQTT_InitConnection(&mqttClient, wmConfig.mqttBroker, MQTT_PORT,
			DEFAULT_SECURITY);
	os_sprintf(client_id, "%s-%s", MODULE_NAME, getMacAddress(STATION_IF));
	MQTT_InitClient(&mqttClient, client_id, wmConfig.mqttUser,
			wmConfig.mqttPassword, 120, 1);
//	MQTT_InitLWT(&mqttClient, "/lwt", "offline", 0, 0);
	MQTT_OnConnected(&mqttClient, mqttConnectedCb);
	MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);
	MQTT_OnData(&mqttClient, mqttDataCb);

}

void ICACHE_FLASH_ATTR mqttInitTimer_cb() {
	mqtt_init();
	if (wifi_station_get_connect_status() == STATION_GOT_IP) {
		mqttConnect();
	}
}

void ICACHE_FLASH_ATTR mqttDelClientTimer_cb() {
	MQTT_DeleteClient(&mqttClient);

	os_timer_disarm(&mqttInitTimer);
	os_timer_setfn(&mqttInitTimer, (os_timer_func_t *) mqttInitTimer_cb,
			(void *) 0);
	os_timer_arm(&mqttInitTimer, 1000, false);
}

void ICACHE_FLASH_ATTR mqttReInit() {
	MQTT_Disconnect(&mqttClient);

	os_timer_disarm(&mqttDelClientTimer);
	os_timer_setfn(&mqttDelClientTimer,
			(os_timer_func_t *) mqttDelClientTimer_cb, (void *) 0);
	os_timer_arm(&mqttDelClientTimer, 1000, false);
}

