#include "esp8266.h"
#include "user_interface.h"
#include "driver/uart.h"

#include "user_config.h"
#include "ssd1306.h"

#include "httpd.h"
#include "webpages-espfs.h"
#include "httpdespfs.h"
#include "cgiflash.h"

#include "wm_utils.h"
#include "wm_mqtt.h"
#include "wm_web.h"

extern _waterCounter waterCounter;

const char* watermeterDirName = WM_DIR;
const char* configFileName = WM_CONFIG_FILE;

LOCAL os_timer_t buttonTimer, sleepTimer, mainTimer;

_config wmConfig;
_waterCounter waterCounter;

/* flags */
bool rebootNow;
bool restartWiFi;
bool sleepNow;
bool dontSleep;
bool powerLow;
bool apModeNow;
bool staModeNow;
bool staApModeNow;
bool staConfigure;
bool defaultConfig;
bool saveNewConfig;
bool firstStart;
bool responseNTP;
bool mqttRestart;
bool mqttFirstStart;
bool mqttConnected;
bool subsHotWater;
bool subsColdWater;
bool firstNTP;
bool sd_ok;
bool displayWrite;

#ifndef OTA_FLASH_MAP
#define OTA_FLASH_MAP 6
#define OTA_FLASH_SIZE_K 4096
#endif

CgiUploadFlashDef uploadParams={
	.type=CGIFLASH_TYPE_FW,
	.fw1Pos=0x1000,
#if OTA_FLASH_MAP == 6 || OTA_FLASH_MAP == 3
	.fw2Pos=((OTA_FLASH_SIZE_K*512)/2)+0x1000,
	.fwSize=((OTA_FLASH_SIZE_K*512)/2)-0x7000,
#else
	#if OTA_FLASH_MAP == 4
	.fw2Pos=((OTA_FLASH_MAP/2*512)/2)+0x1000,
	.fwSize=((OTA_FLASH_MAP/2*512)/2)-0x7000,
	#else
	.fw2Pos=((OTA_FLASH_MAP*1024)/2)+0x1000,
	.fwSize=((OTA_FLASH_MAP*1024)/2)-0x7000,
	#endif
#endif
	.tagName=OTA_TAGNAME
};

HttpdBuiltInUrl builtInUrls[]={
	{"/", cgiRedirect, INDEX_TPL},
	{INDEX_TPL, myCgiEspFsTemplate, tplToken},
	{"/config", cgiRedirect, "/config.tpl"},
	{"/config.tpl", myCgiEspFsTemplate, tplToken},
	{"/upload", cgiRedirect, "/upload.tpl"},
	{"/upload.tpl", myCgiEspFsTemplate, tplToken},
	{"/update", myCgiUpdate, &uploadParams},
	{"/settings", htmlSettings, NULL},
	{"*", cgiEspFsHook, NULL}, //Catch-all cgi function for the filesystem
	{NULL, NULL, NULL}
};


LOCAL void ICACHE_FLASH_ATTR sleepTimer_cb() {

	uint8 bit;

	bit = gpio_read(EXT_POWER_PIN);

	if (bit) {
		powerLow = false;
		if (sleepNow) {
			INFO("External power high.\n");
			sleepNow = false;
		}
		return;
	} else {
		powerLow = true;
		if (!sleepNow) {
			sleepNow = true;
			if (!dontSleep) {
				INFO("Power low. Light sleep now ...\n");
				os_delay_us(50000);
				sleepOnNow();
			}
		} else {
			if (!dontSleep) sleepOnNow();
		}

	}

}


LOCAL uint8_t ICACHE_FLASH_ATTR debounce(uint8 sample) {

  static uint8 state = 0xFF, cnt0, cnt1;
  uint8 delta;

  delta = sample ^ state;
  cnt1 = (cnt1 ^ cnt0) & delta;
  cnt0 = ~cnt0 & delta;
  state ^= (cnt0 & cnt1);

  return state;
}

LOCAL void ICACHE_FLASH_ATTR buttonTimer_cb(void *arg) {

	uint8 bitHot, bitCold, bitStatus, counter = 0;

	bitHot = gpio_read(HOT_PIN);
	bitCold = gpio_read(COLD_PIN) << 1;

	counter |= bitHot;
	counter |= bitCold;

	waterCounter.status = debounce(counter);

	if (waterCounter.status & 0x01) {
		if (!(waterCounter.status_old & 0x01)) {
			waterCounter.status_old |= 0x01;
			waterCounter.counterHotWater++;
			dontSleep = false;
			os_printf("hotCounter  = %d\n", waterCounter.counterHotWater);
		}
	} else {
		if (waterCounter.status_old & 0x01) {
			waterCounter.status_old &= ~0x01;
		}
	}

	if (waterCounter.status & 0x02) {
		if (!(waterCounter.status_old & 0x02)) {
			waterCounter.status_old |= 0x02;
			waterCounter.counterColdWater++;
			dontSleep = false;
			os_printf("coldCounter  = %d\n", waterCounter.counterColdWater);
		}
	} else {
		if (waterCounter.status_old & 0x02) {
			waterCounter.status_old &= ~0x02;
		}
	}
}



LOCAL void ICACHE_FLASH_ATTR mainTimer_cb() {

	char *topic, buff[32];

	os_timer_disarm(&mainTimer);


	/* If counter of hot water was added */
	if (waterCounter.counterHotWater) {

		displayWrite = true;

		topic = (char*) os_malloc(strlen(wmConfig.mqttTopic) + SIZE_END_TOPIC);

		if (!topic) {
			INFO("Error allocation memory in %lu line in %s file\n", __LINE__, __FILE__);
		}

		wmConfig.hotTime = localTimeT();
		wmConfig.hotWater += waterCounter.counterHotWater * wmConfig.litersPerPulse;
		waterCounter.counterHotWater = 0;

		if (topic) {
			os_sprintf(buff, "%lu %lu", wmConfig.hotTime, wmConfig.hotWater);
			os_sprintf(topic, "/%s/%s/%s", wmConfig.mqttTopic, getMacAddress(STATION_IF), END_TOPIC_HOT_OUT);

			INFO("%s <== %s\n", topic, buff);

			if (mqttConnected) {
				if (!mqttPublish(topic, buff, strlen(buff))) {
				saveConfig();
				}
			} else saveConfig();

			os_free(topic);

		} else saveConfig();
	}

	/* If counter of cold water was added */
	if (waterCounter.counterColdWater) {

		displayWrite = true;

		topic = (char*) os_malloc(strlen(wmConfig.mqttTopic) + SIZE_END_TOPIC);

		if (!topic) {
			INFO("Error allocation memory in %lu line in %s file\n", __LINE__, __FILE__);
		}


	    wmConfig.coldTime = localTimeT();
	    wmConfig.coldWater += waterCounter.counterColdWater * wmConfig.litersPerPulse;
		waterCounter.counterColdWater = 0;

		if (topic) {
			os_sprintf(buff, "%lu %lu", wmConfig.coldTime, wmConfig.coldWater);
			os_sprintf(topic, "/%s/%s/%s", wmConfig.mqttTopic, getMacAddress(STATION_IF), END_TOPIC_COLD_OUT);

			INFO("%s <== %s\n", topic, buff);

			if (mqttConnected) {
				if (!mqttPublish(topic, buff, strlen(buff))) {
					saveConfig();
				}
			} else saveConfig();

			os_free(topic);

		} else saveConfig();
	}

	/* If reconfigured counter of hot water */
	if (subsHotWater) {

		displayWrite = true;

		topic = (char*)os_malloc(strlen(wmConfig.mqttTopic) + SIZE_END_TOPIC);

		if (!topic) {
			INFO("Error allocation memory in %lu line in %s file\n", __LINE__, __FILE__);
		} else {
			subsHotWater = false;
			os_sprintf(buff, "%lu %lu NEW", wmConfig.hotTime, wmConfig.hotWater);
			os_sprintf(topic, "/%s/%s/%s", wmConfig.mqttTopic, getMacAddress(STATION_IF), END_TOPIC_HOT_OUT);

			INFO("%s <== %s\n", topic, buff);

			if (mqttConnected) mqttPublish(topic, buff, strlen(buff));
			os_free(topic);
		}

	}

	/* If reconfigured counter of cold water */
	if (subsColdWater) {

		displayWrite = true;

		topic = (char*)os_malloc(strlen(wmConfig.mqttTopic) + SIZE_END_TOPIC);

		if (!topic) {
			INFO("Error allocation memory in %lu line in %s file\n", __LINE__, __FILE__);
		} else {
			subsColdWater = false;
			os_sprintf(buff, "%lu %lu NEW", wmConfig.coldTime, wmConfig.coldWater);
			os_sprintf(topic, "/%s/%s/%s", wmConfig.mqttTopic, getMacAddress(STATION_IF), END_TOPIC_COLD_OUT);

			INFO("%s <== %s\n", topic, buff);

			if (mqttConnected) mqttPublish(topic, buff, strlen(buff));
			os_free(topic);
		}

	}

	if (displayWrite) {
		displayUpdate();
	}

	/* Set time of starting system from real time */
	if (firstNTP && responseNTP) {
		setTimeStart(sntp_get_current_timestamp());
		firstNTP = false;
	}

	//os_timer_setfn(&mainTimer, (os_timer_func_t *)mainTimer_cb, (void *)0);
	os_timer_arm(&mainTimer, 250, false);
}


LOCAL void ICACHE_FLASH_ATTR setup_cb() {

	setNullWiFiConfigDefault();

	if (firstStart || wmConfig.apMode || wmConfig.staSsid[0] == '0') {
		if (!sleepNow) {
			startWiFiAP();
			startApMsg();
		}
	} else {
		if (!sleepNow) {
			startWiFiSTA();
		}
	}

	mqttFirstStart = true;

	httpdInit(builtInUrls, 80);

	os_timer_disarm(&mainTimer);
	os_timer_setfn(&mainTimer, (os_timer_func_t *)mainTimer_cb, (void *)0);
	os_timer_arm(&mainTimer, 250, false);

	sntp_setservername(0, wmConfig.ntpServerName);//	set	server	0	by	domain	name

	sntp_init();

	mqtt_init();

	displayUpdate();
}

void ICACHE_FLASH_ATTR sleepTimerOn() {

	if (SLEEP_MODE_ON) {
		os_timer_disarm(&sleepTimer);
		os_timer_setfn(&sleepTimer, (os_timer_func_t *) sleepTimer_cb, (void *) 0);
		os_timer_arm(&sleepTimer, 2000, true);
	}

}


/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABBBCDDD
 *                A : rf cal
 *                B : at parameters
 *                C : rf init data
 *                D : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32 ICACHE_FLASH_ATTR user_rf_cal_sector_set(void)
{
    enum flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 8;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

void ICACHE_FLASH_ATTR user_rf_pre_init(void)
{
	Cache_Read_Enable(0, 0, 1);
}

void ICACHE_FLASH_ATTR user_init(void)
{

	setTimeStart(system_get_rtc_time());

	if (DEBUG)
		uart_div_modify(0, UART_CLK_FREQ / 115200);
	else
		uart_init(BIT_RATE_115200, BIT_RATE_115200);

	rebootNow = false;
	restartWiFi = false;
	sleepNow = false;
	dontSleep = false;
	powerLow = false;
	apModeNow = true;
	staModeNow = false;
	staApModeNow = false;
	staConfigure = false;
	defaultConfig = false;
	saveNewConfig = false;
	firstStart = false;
	responseNTP = false;
	mqttRestart = false;
	mqttFirstStart = true;
	mqttConnected = false;
	subsHotWater = false;
	subsColdWater = false;
	firstNTP = true;
	sd_ok = false;
	displayWrite = false;

	os_delay_us(50000);

	INFO("\n\n\n");

	INFO("SDK version:%s\n", system_get_sdk_version());

	set107init_data_bin();

	/* init waterCounter structure */
	waterCounter.status = waterCounter.status_old = 3;
	waterCounter.counterHotWater = waterCounter.counterColdWater = 0;

	/* init I2C for oled ssd1306 */
	i2c_master_gpio_init ();

	display_init(I2C_ADDR);
	os_delay_us(5000);
	display_clear();
	display_update();

	/* init pin for hot water, cold water and external power */
	if (set_gpio_mode(HOT_PIN, GPIO_INPUT, GPIO_PULLUP)) {
		os_printf("GPIO%d set input mode\n", pin_num[HOT_PIN]);
	}

	if (set_gpio_mode(COLD_PIN, GPIO_INPUT, GPIO_PULLUP)) {
		os_printf("GPIO%d set input mode\n", pin_num[COLD_PIN]);
	}

	if (set_gpio_mode(EXT_POWER_PIN, GPIO_INPUT, GPIO_PULLDOWN)) {
		os_printf("GPIO%d set input mode\n", pin_num[EXT_POWER_PIN]);
	}

	/* Start timer to monitor button for hot and cold water */
	os_timer_disarm(&buttonTimer);
	os_timer_setfn(&buttonTimer, (os_timer_func_t *)buttonTimer_cb, (void *)0);
	os_timer_arm(&buttonTimer, 50, true);

	spi_init(spi_no);
	sd_init();

	espFsInit((void*)(webpages_espfs_start));

	initDefConfig(&wmConfig);
//	printConfig(&wmConfig);


	if (!readConfig()) {
		firstStart = true;
		if (sd_ok) {
			mkDir(watermeterDirName);
		}
	}

	system_init_done_cb(setup_cb);

}
