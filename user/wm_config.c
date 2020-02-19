#include "esp8266.h"
#include "user_interface.h"
#include "user_config.h"
#include "spi_flash.h"

#include "global.h"
#include "wm_config.h"
#include "wm_wifi.h"
#include "wm_utils.h"
#include "wm_mqtt.h"
#include "fat_filelib.h"

#ifndef OTA_FLASH_MAP
#define OTA_FLASH_MAP 6
#endif

#if OTA_FLASH_MAP == 0
#define SEC_FLASH 0x7a
#else
	#if OTA_FLASH_MAP == 2 || OTA_FLASH_MAP == 3 || OTA_FLASH_MAP == 4
#define SEC_FLASH 0xfa
	#else
#define SEC_FLASH 0x1fa
	#endif
#endif

#define CRC_ERROR 4

uint32 crc_table[16] = {
  0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac, 0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
  0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c, 0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
};

LOCAL uint32 ICACHE_FLASH_ATTR crc_update(uint8* buf, uint32 len) {
  unsigned long crc = ~0L;
  int i;

  for (i = 0; i < len; i++) {
    crc = crc_table[(crc ^ buf[i]) & 0x0f] ^ (crc >> 4);
    crc = crc_table[(crc ^ (buf[i] >> 4)) & 0x0f] ^ (crc >> 4);
    crc = ~crc;
  }
  return crc;
}


LOCAL SpiFlashOpResult ICACHE_FLASH_ATTR read_flash_config(uint8* buffer, uint32 len) {

	SpiFlashOpResult ret;
	uint32 crc_flash, crc, crc_len;
	uint8* buf;

	crc_len = sizeof(crc);

	buf = (uint8*)os_malloc(len+crc_len);

	if (!buf) {
		INFO("Error allocation memory in %lu line in %s file\n", __LINE__, __FILE__);
		return -1;
	}

	os_bzero(buf, len+crc_len);

	ets_intr_lock();

	ret = spi_flash_read(SEC_FLASH*SPI_FLASH_SEC_SIZE, (uint32*)buf, len+crc_len);
	if (ret == 	SPI_FLASH_RESULT_OK) {
		os_memcpy(buffer, buf, len);
		os_memcpy(&crc_flash, buf+len, crc_len);
		crc = crc_update(buffer, len);
		if (crc != crc_flash) ret = CRC_ERROR;
	}

	ets_intr_unlock();

	os_free(buf);

	return ret;

}

LOCAL SpiFlashOpResult ICACHE_FLASH_ATTR write_flash_config(uint8* buffer, uint32 len) {

	SpiFlashOpResult ret;
	uint32 crc, crc_len;
	uint8* buf;

	crc = crc_update(buffer, len);
	crc_len = sizeof(crc);
	buf = (uint8*)os_malloc(len+crc_len);

	if (!buf) {
		INFO("Error allocation memory in %lu line in %s file\n", __LINE__, __FILE__);
		return -1;
	}

	os_memcpy(buf, buffer, len);
	os_memcpy(buf+len, &crc, crc_len);

	ets_intr_lock();

    spi_flash_erase_sector(SEC_FLASH);

	ret = spi_flash_write(SEC_FLASH*SPI_FLASH_SEC_SIZE, (uint32*)buf, len+crc_len);

	ets_intr_unlock();

	os_free(buf);

	return ret;
}

LOCAL void ICACHE_FLASH_ATTR clear_flash_config() {

	uint8 *buff;

	buff = (uint8*)os_malloc(sizeof(_config)+10);

	if (!buff) {
		INFO("Error allocation memory in %lu line in %s file\n", __LINE__, __FILE__);
		return;
	}

	os_bzero(buff, sizeof(_config)+10);

	write_flash_config(buff, sizeof(_config)+10);

	os_free(buff);
}

void ICACHE_FLASH_ATTR removeConfig() {
	if (sd_ok) fl_remove(configFileName);
	clear_flash_config();
}

void ICACHE_FLASH_ATTR clearConfig(_config *config) {
	os_bzero(config, sizeof(_config));
}

void ICACHE_FLASH_ATTR initDefConfig(_config *config) {

	clearConfig(config);
	os_strcpy(config->webAdminLogin, WEB_ADMIN_LOGIN);
	os_strcpy(config->webAdminPassword, WEB_ADMIN_PASSWORD);
	config->fullSecurity = false;
	config->configSecurity = false;
	os_bzero(config->staSsid, sizeof(config->staSsid));
	os_bzero(config->staPassword, sizeof(config->staPassword));
	config->apMode = true;
	os_strcpy(config->apSsid, AP_SSID);
	os_strcpy(config->apPassword, AP_PASSWORD);
	os_strcpy(config->mqttBroker, MQTT_BROKER);
	os_strcpy(config->mqttUser, MQTT_USER);
	os_strcpy(config->mqttPassword, MQTT_PASSWORD);
	os_strcpy(config->mqttTopic, MQTT_TOPIC);
	os_strcpy(config->ntpServerName, NTP_SERVER_NAME);
	config->timeZone = TIME_ZONE;
	config->litersPerPulse = LITERS_PER_PULSE;
	config->hotTime = config->coldTime = localTimeT();
	config->hotWater = config->coldWater = 0;

	staConfigure = false;
}

/* copy from tmp _config to wmConfig */

void ICACHE_FLASH_ATTR setConfig(_config *config) {

	clearConfig(&wmConfig);

	os_strcpy(wmConfig.webAdminLogin, config->webAdminLogin);
	os_strcpy(wmConfig.webAdminPassword, config->webAdminPassword);
	wmConfig.fullSecurity = config->fullSecurity;
	wmConfig.configSecurity = config->configSecurity;
	os_strcpy(wmConfig.staSsid, config->staSsid);
	os_strcpy(wmConfig.staPassword, config->staPassword);
	wmConfig.apMode = config->apMode;
	os_strcpy(wmConfig.apSsid, config->apSsid);
	os_strcpy(wmConfig.apPassword, config->apPassword);
	os_strcpy(wmConfig.mqttBroker, config->mqttBroker);
	os_strcpy(wmConfig.mqttUser, config->mqttUser);
	os_strcpy(wmConfig.mqttPassword, config->mqttPassword);
	os_strcpy(wmConfig.mqttTopic, config->mqttTopic);
	os_strcpy(wmConfig.ntpServerName, config->ntpServerName);
	wmConfig.timeZone = config->timeZone;
	wmConfig.litersPerPulse = config->litersPerPulse;
	wmConfig.hotTime = config->hotTime;
	wmConfig.coldTime = config->coldTime;
	wmConfig.hotWater = config->hotWater;
	wmConfig.coldWater = config->coldWater;

}


void ICACHE_FLASH_ATTR printConfig(_config* config) {
	os_printf("webAdminLogin: \"%s\"\n", config->webAdminLogin);

	os_printf("webAdminPassword: \"%s\"\n", config->webAdminPassword);
	os_printf("fullSecurity: %s\n", config->fullSecurity?"true":"false");
	os_printf("configSecurity: %s\n", config->configSecurity?"true":"false");
	os_printf("staSsid: \"%s\"\n", config->staSsid);
	os_printf("staPassword: \"%s\"\n", config->staPassword);
	os_printf("apMode: %s\n", config->apMode?"true":"false");
	os_printf("apSsid: \"%s\"\n", config->apSsid);
	os_printf("apPassword: \"%s\"\n", config->apPassword);
	os_printf("mqttBroker: \"%s\"\n", config->mqttBroker);
	os_printf("mqttUser: \"%s\"\n", config->mqttUser);
	os_printf("mqttPassword: \"%s\"\n", config->mqttPassword);
	os_printf("mqttTopic: \"%s\"\n", config->mqttTopic);
	os_printf("ntpServerName: \"%s\"\n", config->ntpServerName);
	os_printf("timeZone: %d\n", config->timeZone);
	os_printf("litersPerPulse: %d\n", config->litersPerPulse);
	os_printf("hotTime: %d\n", config->hotTime);
	os_printf("coldTime: %d\n", config->coldTime);
	os_printf("hotWater: %d\n", config->hotWater);
	os_printf("coldWater: %d\n", config->coldWater);


}

bool ICACHE_FLASH_ATTR readConfig() {
//  return false;
	_config config;
	bool ret = false;

	if (sd_ok) {
		os_printf("Read from SD. File name: %s\n", configFileName);
		/* read from SD */
		FL_FILE *file = fl_fopen(configFileName, "rb");
		if (file) {
			fl_fseek(file, 0, 0);
			if (fl_fread(&config, 1, sizeof(_config), file) != sizeof(_config)) {
				INFO("Error read config from SD!\n");
				fl_fclose(file);
				return ret;
			}
			fl_fclose(file);
			setConfig(&config);
			if (wmConfig.staSsid[0] != 0)
				staConfigure = true;
			ret = true;
		} else {
				INFO("readConfigFile() - can't read from config file: %s\n", configFileName);
			ret = false;
		}
	} else {
		/* read from EEPROM */
		if (NOT_READ_EEPROM)
			return false;
		if (read_flash_config((uint8*)&config, sizeof(_config)) == SPI_FLASH_RESULT_OK) {
			os_printf("Read from EEPROM true\n");
			setConfig(&config);
			if (wmConfig.staSsid[0] != 0)
				staConfigure = true;
			ret = true;
		} else {
			INFO("Read from EEPROM false\n");
			ret = false;
		}
	}

	return ret;
}

void ICACHE_FLASH_ATTR saveConfig() {
	if (sd_ok) {
		os_printf("Save to SD. File name: %s\n", configFileName);
		FL_FILE *file = fl_fopen(configFileName, "wb");
		if (file) {
			fl_fseek(file, 0, 0);
			if (fl_fwrite(&wmConfig, 1, sizeof(_config), file) != sizeof(_config)) {
				INFO("Error write config to SD!\n");
				fl_fclose(file);
				return;
			}
			fl_fclose(file);
		} else
			os_printf("saveConfigFile() - can\'t write to config file: %s\n", configFileName);
	} else {
		os_printf("Save to flash\n");
		write_flash_config((uint8*)&wmConfig, sizeof(_config));
	}
}

