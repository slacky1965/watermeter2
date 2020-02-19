#include "osapi.h"
#include "os_type.h"
#include "mem.h"
#include "upgrade.h"

#include "platform.h"
#include "cgiflash.h"
#include "global.h"
#include "wm_web.h"
#include "wm_utils.h"
//#include "user_config.h"
#include "fat_filelib.h"

int ICACHE_FLASH_ATTR tplToken(HttpdConnData *connData, char *token, void **arg) {
	char buff[128], *tmp;
	if (token == NULL)
		return HTTPD_CGI_DONE;

	os_strcpy(buff, "Unknown");
	if (os_strcmp(token, "firstname") == 0) {
		os_strcpy(buff, WEB_WATERMETER_FIRST_NAME);
	} else if (os_strcmp(token, "lastname") == 0) {
		os_strcpy(buff, WEB_WATERMETER_LAST_NAME);
	} else if (os_strcmp(token, "platform") == 0) {
		os_strcpy(buff, PLATFORM);
	} else if (os_strcmp(token, "modulename") == 0) {
		os_strcpy(buff, MODULE_NAME);
	} else if (os_strcmp(token, "version") == 0) {
		os_strcpy(buff, MODULE_NAME);
	} else if (os_strcmp(token, "heap") == 0) {
		os_sprintf(buff, "%d", system_get_free_heap_size());
	} else if (os_strcmp(token, "uptime") == 0) {
		os_strcpy(buff, localUpTime());
	} else if (os_strcmp(token, "volt") == 0) {
		os_strcpy(buff, getVoltage());
	} else if (os_strcmp(token, "rssi") == 0) {
		os_strcpy(buff, getRssi());
	} else if (os_strcmp(token, "localtime") == 0) {
		os_strcpy(buff, localTimeStr());
	} else if (os_strcmp(token, "adminlogin") == 0) {
		os_strcpy(buff, wmConfig.webAdminLogin);
	} else if (os_strcmp(token, "adminpassword") == 0) {
		os_strcpy(buff, wmConfig.webAdminPassword);
	} else if (os_strcmp(token, "fullsecurity") == 0) {
		if (wmConfig.fullSecurity)
			os_strcpy(buff, "checked");
	} else if (os_strcmp(token, "configsecurity") == 0) {
		if (wmConfig.configSecurity)
			os_strcpy(buff, "checked");
	} else if (os_strcmp(token, "wifistamode") == 0) {
		if (!wmConfig.apMode)
			os_strcpy(buff, "checked");
	} else if (os_strcmp(token, "wifiapmode") == 0) {
		if (wmConfig.apMode)
			os_strcpy(buff, "checked");
	} else if (os_strcmp(token, "wifiapname") == 0) {
		os_strcpy(buff, wmConfig.apSsid);
	} else if (os_strcmp(token, "wifiappassword") == 0) {
		os_strcpy(buff, wmConfig.apPassword);
	} else if (os_strcmp(token, "wifistaname") == 0) {
		os_strcpy(buff, wmConfig.staSsid);
	} else if (os_strcmp(token, "wifistapassword") == 0) {
		os_strcpy(buff, wmConfig.staPassword);
	} else if (os_strcmp(token, "mqttuser") == 0) {
		os_strcpy(buff, wmConfig.mqttUser);
	} else if (os_strcmp(token, "mqttpassword") == 0) {
		os_strcpy(buff, wmConfig.mqttPassword);
	} else if (os_strcmp(token, "mqttbroker") == 0) {
		os_strcpy(buff, wmConfig.mqttBroker);
	} else if (os_strcmp(token, "mqtttopic") == 0) {
		os_strcpy(buff, wmConfig.mqttTopic);
	} else if (os_strcmp(token, "ntpserver") == 0) {
		os_strcpy(buff, wmConfig.ntpServerName);
	} else if (os_strcmp(token, "timezone") == 0) {
		os_sprintf(buff, "%d", wmConfig.timeZone);
	} else if (os_strcmp(token, "hotwater") == 0) {
		os_sprintf(buff, "%lu", wmConfig.hotWater);
	} else if (os_strcmp(token, "coldwater") == 0) {
		os_sprintf(buff, "%lu", wmConfig.coldWater);
	} else if (os_strcmp(token, "litersperpulse") == 0) {
		os_sprintf(buff, "%d", wmConfig.litersPerPulse);
	} else if (os_strcmp(token, "hotwhole") == 0) {
		os_sprintf(buff, "%lu", wmConfig.hotWater / 1000);
	} else if (os_strcmp(token, "hotfract") == 0) {
		os_sprintf(buff, "%lu", wmConfig.hotWater % 1000);
	} else if (os_strcmp(token, "coldwhole") == 0) {
		os_sprintf(buff, "%lu", wmConfig.coldWater / 1000);
	} else if (os_strcmp(token, "coldfract") == 0) {
		os_sprintf(buff, "%lu", wmConfig.coldWater % 1000);
	} else if (os_strcmp(token, "upgfile") == 0) {
		if (system_upgrade_userbin_check() == 1)
			os_strcpy(buff, "user1.bin");
		else
			os_strcpy(buff, "user2.bin");
	}

	httpdSend(connData, buff, -1);

	return HTTPD_CGI_DONE;
}

int ICACHE_FLASH_ATTR htmlSettings(HttpdConnData *connData) {

	char *p, *arg;
	char sep[] = "&";
	char buff[16];
	_config config;

	bool newSave = false;

	clearConfig(&config);

	if (!connData->getArgs)
		return HTTPD_CGI_DONE;

	p = (char*) strtok(connData->getArgs, sep);

	while (p) {
		arg = (char*) os_strstr(p, "login");
		if (arg) {
			arg += 6;
			os_strcpy(config.webAdminLogin, arg);
			p = (char*) strtok(NULL, sep);
			continue;
		}

		arg = (char*) os_strstr(p, "password");
		if (arg) {
			arg += 9;
			os_strcpy(config.webAdminPassword, arg);
			p = (char*) strtok(NULL, sep);
			continue;
		}

		arg = (char*) os_strstr(p, "fullsecurity");
		if (arg) {
			if (strstr(arg, "true")) {
				config.fullSecurity = true;
			}
			p = (char*) strtok(NULL, sep);
			continue;
		}

		arg = (char*) os_strstr(p, "configsecurity");
		if (arg) {
			if (strstr(arg, "true")) {
				config.configSecurity = true;
			}
			p = (char*) strtok(NULL, sep);
			continue;
		}

		arg = (char*) os_strstr(p, "wifi-mode");
		if (arg) {
			if (strstr(arg, "station"))
				config.apMode = false;
			if (strstr(arg, "ap"))
				config.apMode = true;
			p = (char*) strtok(NULL, sep);
			continue;
		}

		arg = (char*) os_strstr(p, "wifi-ap-name");
		if (arg) {
			arg += 13;
			os_strcpy(config.apSsid, arg);
			p = (char*) strtok(NULL, sep);
			continue;
		}

		arg = (char*) os_strstr(p, "wifi-ap-pass");
		if (arg) {
			arg += 13;
			os_strcpy(config.apPassword, arg);
			p = (char*) strtok(NULL, sep);
			continue;
		}

		arg = (char*) os_strstr(p, "wifi-sta-name");
		if (arg) {
			arg += 14;
			os_strcpy(config.staSsid, arg);
			p = (char*) strtok(NULL, sep);
			continue;
		}

		arg = (char*) os_strstr(p, "wifi-sta-pass");
		if (arg) {
			arg += 14;
			os_strcpy(config.staPassword, arg);
			p = (char*) strtok(NULL, sep);
			continue;
		}

		arg = (char*) os_strstr(p, "mqtt-user");
		if (arg) {
			arg += 10;
			os_strcpy(config.mqttUser, arg);
			p = (char*) strtok(NULL, sep);
			continue;
		}

		arg = (char*) os_strstr(p, "mqtt-pass");
		if (arg) {
			arg += 10;
			os_strcpy(config.mqttPassword, arg);
			p = (char*) strtok(NULL, sep);
			continue;
		}

		arg = (char*) os_strstr(p, "mqtt-broker");
		if (arg) {
			arg += 12;
			os_strcpy(config.mqttBroker, arg);
			p = (char*) strtok(NULL, sep);
			continue;
		}

		arg = (char*) os_strstr(p, "mqtt-topic");
		if (arg) {
			arg += 11;
			os_strcpy(config.mqttTopic, arg);
			p = (char*) strtok(NULL, sep);
			continue;
		}

		arg = (char*) os_strstr(p, "serv-ntp");
		if (arg) {
			arg += 9;
			os_strcpy(config.ntpServerName, arg);
			p = (char*) strtok(NULL, sep);
			continue;
		}

		arg = (char*) os_strstr(p, "gmt-zone");
		if (arg) {
			arg += 9;
			config.timeZone = atoi(arg);
			p = (char*) strtok(NULL, sep);
			continue;
		}

		arg = (char*) os_strstr(p, "hotw");
		if (arg) {
			arg += 5;
			config.hotTime = localTimeT();
			config.hotWater = strtoul(arg, 0, 10);
			p = (char*) strtok(NULL, sep);
			continue;
		}

		arg = (char*) os_strstr(p, "coldw");
		if (arg) {
			arg += 6;
			config.coldTime = localTimeT();
			config.coldWater = strtoul(arg, 0, 10);
			p = (char*) strtok(NULL, sep);
			continue;
		}

		arg = (char*) os_strstr(p, "countw");
		if (arg) {
			arg += 7;
			config.litersPerPulse = atoi(arg);
			p = (char*) strtok(NULL, sep);
			continue;
		}

		arg = (char*) os_strstr(p, "reboot");
		if (arg) {
			rebootNow = true;
			p = (char*) strtok(NULL, sep);
			continue;
		}

		arg = (char*) os_strstr(p, "defconfig");
		if (arg) {
			defaultConfig = true;
			p = (char*) strtok(NULL, sep);
			continue;
		}

		arg = (char*) os_strstr(p, "set-conf");
		if (arg) {
			if (strstr(arg, "set")) {
				saveNewConfig = true;
			}
			p = (char*) strtok(NULL, sep);
			continue;
		}

		p = (char*) strtok(NULL, sep);

	}

//	printConfig(&config);

	if (defaultConfig) {
		defaultConfig = false;
		removeConfig();
		if (rebootNow) {
			INFO("Rebooting ...\n");
			os_delay_us(50000);
			system_restart();
		}
		os_delay_us(50000);

		firstStart = true;
		initDefConfig(&wmConfig);

		if (sd_ok) {
			mkDir(watermeterDirName);
		}

		startWiFiAP();
		startApMsg();

		httpdEndHeaders(connData);
		return HTTPD_CGI_DONE;
	}

	if (saveNewConfig) {

		if (strcmp(wmConfig.webAdminLogin, config.webAdminLogin) != 0 ||
		strcmp(wmConfig.webAdminPassword, config.webAdminPassword) != 0
				|| wmConfig.fullSecurity != config.fullSecurity
				|| wmConfig.configSecurity != config.configSecurity)
			newSave = true;

		if (apModeNow) {
			if (!config.apMode || strcmp(wmConfig.apSsid, config.apSsid) != 0 ||
			strcmp(wmConfig.apPassword, config.apPassword) != 0) {
				restartWiFi = true;
				newSave = true;
			}
		}

		if (staModeNow) {
			if (config.apMode || strcmp(wmConfig.staSsid, config.staSsid) != 0
					||
					strcmp(wmConfig.staPassword, config.staPassword) != 0) {
				restartWiFi = true;
				staConfigure = true;
				newSave = true;
			}
		}

		if (strcmp(wmConfig.ntpServerName, config.ntpServerName) != 0
				|| wmConfig.timeZone != config.timeZone) {
			responseNTP = false;
			newSave = true;
		}

		if (strcmp(wmConfig.mqttBroker, config.mqttBroker) != 0
				|| strcmp(wmConfig.mqttUser, config.mqttUser) != 0 ||
				strcmp(wmConfig.mqttPassword, config.mqttPassword) != 0
				|| strcmp(wmConfig.mqttTopic, config.mqttTopic) != 0) {
			mqttRestart = true;
			newSave = true;
		}

		if (wmConfig.hotWater != config.hotWater) {
			subsHotWater = true;
			newSave = true;
		}
		if (wmConfig.coldWater != config.coldWater) {
			subsColdWater = true;
			newSave = true;
		}

		if (newSave) {
			setConfig(&config);
			saveConfig();
			firstStart = false;
		}

		if (restartWiFi) {
			if (wmConfig.apMode || wmConfig.staSsid[0] == '0') {
				staConfigure = false;
				startWiFiAP();
				startApMsg();
			} else {
				startWiFiSTA();
			}
		}

		if (mqttRestart) {
			mqttRestart = false;
			mqttReInit();
		}

		saveNewConfig = false;
	}

	if (rebootNow) {
		INFO("Rebooting ...\n");
		os_delay_us(50000);
		system_restart();
	}

	httpdEndHeaders(connData);
	return HTTPD_CGI_DONE;
}

LOCAL int ICACHE_FLASH_ATTR adminPass(HttpdConnData *connData, int no,
		char *user, int userLen, char *pass, int passLen) {

	if (no == 0) {
		os_strcpy(user, wmConfig.webAdminLogin);
		os_strcpy(pass, wmConfig.webAdminPassword);
		return 1;
	}

	return 0;

}

int ICACHE_FLASH_ATTR myCgiEspFsTemplate(HttpdConnData *connData) {

	const void *cgiArg = connData->cgiArg;
	int (*newCgiArg)(HttpdConnData *connData, int no, char *user, int userLen,
			char *pass, int passLen);

	newCgiArg = adminPass;

	connData->cgiArg = newCgiArg;

	if (((wmConfig.fullSecurity && os_strcmp(INDEX_TPL, connData->url) == 0)
			|| (wmConfig.configSecurity || wmConfig.fullSecurity))
					&& (os_strcmp(INDEX_TPL, connData->url) != 0)) {

		if (authBasic(connData) != HTTPD_CGI_AUTHENTICATED) {
			connData->cgiArg = cgiArg;
			return HTTPD_CGI_DONE;
		}

	}

	connData->cgiArg = cgiArg;

	if (firstStart && os_strcmp(INDEX_TPL, connData->url) == 0) {
		httpdRedirect(connData, "/config.tpl");
		return HTTPD_CGI_DONE;
	}

	return cgiEspFsTemplate(connData);
}

LOCAL os_timer_t resetTimer;

LOCAL void ICACHE_FLASH_ATTR resetTimerCb(void *arg) {
	system_upgrade_flag_set(UPGRADE_FLAG_FINISH);
	system_upgrade_reboot();
}

// Check that the header of the firmware blob looks like actual firmware...
LOCAL int ICACHE_FLASH_ATTR checkBinHeader(void *buf) {
	uint8_t *cd = (uint8_t *) buf;
	if (cd[0] != 0xEA)
		return 0;
	if (cd[1] != 4 || cd[2] > 3 || cd[3] > 0x40)
		return 0;
	if (((uint16_t *) buf)[3] != 0x4010)
		return 0;
	if (((uint32_t *) buf)[2] != 0)
		return 0;
	return 1;
}

#define PAGELEN 256

typedef struct {
	char pageData[PAGELEN];
	int pagePos;
	int address;
	int len;
	FL_FILE *file;
} UploadState;

int ICACHE_FLASH_ATTR myCgiUpdate(HttpdConnData *connData) {

	CgiUploadFlashDef *def = (CgiUploadFlashDef*) connData->cgiArg;
	UploadState *state = (UploadState *) connData->cgiData;

	char *data;
	int dataLen;
	bool lastBuff = false;

	if (connData->conn == NULL) {
		//Connection aborted. Clean up.
		if (state != NULL)
			free(state);
		return HTTPD_CGI_DONE;
	}

	if (state == NULL) {
		//First call. Allocate and initialize state variable.
		os_printf("Firmware upload cgi start.\n");

		const char *ctype = "Content-Type: application/octet-stream\r\n\r\n";
		char *type = (char*) os_strstr(connData->post->buff, ctype);
		char *err = NULL;

		if (!type) {
			err = "Invalid content type\n";
		} else {

			data = type + strlen(ctype);
			dataLen = connData->post->buffLen
					- (type + strlen(ctype) - connData->post->buff);

			if (!checkBinHeader(data)) {
				err = "Invalid flash image type!\n";
			}

			if (connData->post->len > def->fwSize) {
				err = "Firmware image too large\n";
			}
		}

		if (err) {
			os_printf(err);
			httpdStartResponse(connData, 400);
			httpdHeader(connData, "Content-Type", "text/plain");
			httpdEndHeaders(connData);
			httpdSend(connData, err, -1);
			return HTTPD_CGI_DONE;
		}

		state = (UploadState*) os_malloc(sizeof(UploadState));
		if (state == NULL) {
			INFO("Error allocation memory in %lu line in %s file\n", __LINE__, __FILE__);
			os_printf("Can't allocate firmware upload struct!\n");
			return HTTPD_CGI_DONE;
		}
		os_memset(state, 0, sizeof(UploadState));
		connData->cgiData = state;

		if (system_upgrade_userbin_check() == 1) {
			os_printf("Flashing user1.bin\n");
			state->address = def->fw1Pos;
		} else {
			os_printf("Flashing user2.bin\n");
			state->address = def->fw2Pos;
		}

		int countSector = connData->post->len / SPI_FLASH_SEC_SIZE;
		countSector += connData->post->len % SPI_FLASH_SEC_SIZE ? 1 : 0;

		int i, flashSector = state->address / SPI_FLASH_SEC_SIZE;

		ets_intr_lock();

		for (i = 0; i < countSector; i++) {
			spi_flash_erase_sector(flashSector);
			flashSector++;
		}

		ets_intr_unlock();

//		state->file = fl_fopen("/user.bin", "wb");

	} else {
		data = connData->post->buff;
		dataLen = connData->post->buffLen;

		char *boundary = (char*) memmem(data, dataLen,
				connData->post->multipartBoundary,
				strlen(connData->post->multipartBoundary));
		if (boundary) {
			dataLen -= (data + dataLen) - (boundary - 2);
			lastBuff = true;
		}

	}

	while (dataLen != 0) {

		bool write = false;
		if (state->pagePos == 0) {
			if (dataLen >= PAGELEN) {
				os_memcpy(state->pageData, data, PAGELEN);
				write = true;
				state->len = PAGELEN;
				data += PAGELEN;
				dataLen -= PAGELEN;
			} else {
				os_memcpy(state->pageData, data, dataLen);
				state->pagePos = dataLen;
				if (lastBuff) {
					write = true;
					state->len = dataLen;
				}
				dataLen = 0;
			}
		} else {
			int len = PAGELEN - state->pagePos;
			if (dataLen >= len) {
				os_memcpy(&state->pageData[state->pagePos], data, len);
				write = true;
				state->len = PAGELEN;
				data += len;
				dataLen -= len;
				state->pagePos = 0;
			} else {
				os_memcpy(&state->pageData[state->pagePos], data, len);
				state->pagePos += len;
				if (lastBuff) {
					write = true;
					state->len = len;
				}
			}
		}

		if (write) {

			if (state->file) {
				if (fl_fwrite(state->pageData, 1, state->len, state->file)
						!= state->len) {
					os_printf("Error write to SD!\n");
				}

			}

			ets_intr_lock();

			/*			if ((state->address&(SPI_FLASH_SEC_SIZE-1))==0) {
			 spi_flash_erase_sector(state->address/SPI_FLASH_SEC_SIZE);
			 }
			 */
			spi_flash_write(state->address, (uint32 *) state->pageData,
					state->len);

			ets_intr_unlock();

			state->address += state->len;

		}
	}

	if (connData->post->len == connData->post->received) {
		//We're done! Format a response.
		os_printf("Upload done. Sending response.\n");
		httpdStartResponse(connData, 200);
		httpdHeader(connData, "Content-Type", "text/plain");
		httpdEndHeaders(connData);
		httpdSend(connData, "Upload done! Rebooting ...\n", -1);
		if (state->file)
			fl_fclose(state->file);
		free(state);

		os_timer_disarm(&resetTimer);
		os_timer_setfn(&resetTimer, resetTimerCb, NULL);
		os_timer_arm(&resetTimer, 1000, 0);

		os_printf("Rebooting...\n");

		return HTTPD_CGI_DONE;
	}

	return HTTPD_CGI_MORE;

}

