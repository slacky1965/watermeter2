#include "esp8266.h"
#include "user_interface.h"
#include "wm_wifi.h"

#include "global.h"
#include "wm_config.h"
#include "wm_utils.h"
#include "wm_mqtt.h"

LOCAL os_timer_t staTimer;

void ICACHE_FLASH_ATTR startWiFiSTA_AP() {

	struct softap_config ap_config;
	struct station_config sta_config;


	staApModeNow = true;

	INFO("Start WiFi AP+STA Mode\n");

	wifi_station_disconnect();

	wifi_set_phy_mode(PHY_MODE_11G);

	wifi_softap_get_config(&ap_config);
	wifi_station_get_config(&sta_config);

	os_strcpy(ap_config.ssid, wmConfig.apSsid);
	os_strcpy(ap_config.password, wmConfig.apPassword);
	ap_config.ssid_len = os_strlen(wmConfig.apSsid);
	ap_config.authmode = AUTH_WPA_WPA2_PSK;
	os_memset(&sta_config, 0, sizeof(struct station_config));

	wifi_set_opmode_current(STATIONAP_MODE);
	wifi_softap_set_config_current(&ap_config);
	wifi_station_set_config_current(&sta_config);
	wifi_station_dhcpc_start();
	wifi_station_connect();

}

void ICACHE_FLASH_ATTR startWiFiAP() {

	struct softap_config ap_config;

	INFO("Start WiFi AP Mode\n");

	os_timer_disarm(&staTimer);

	wifi_station_disconnect();

	wifi_set_phy_mode(PHY_MODE_11G);

	wifi_softap_get_config(&ap_config);

	os_strcpy(ap_config.ssid, wmConfig.apSsid);
	os_strcpy(ap_config.password, wmConfig.apPassword);
	ap_config.ssid_len = os_strlen(wmConfig.apSsid);
	ap_config.authmode = AUTH_WPA_WPA2_PSK;
	ap_config.max_connection = 5;

	wifi_softap_set_config_current(&ap_config);
	wifi_set_opmode_current(SOFTAP_MODE);

	wifi_station_dhcpc_start();

	wifi_station_connect();

	INFO("wifi_softap_get_station_num = %d\n", wifi_softap_get_station_num());

	restartWiFi = false;
	apModeNow = true;
	staModeNow = false;
	staApModeNow = false;

}

LOCAL void ICACHE_FLASH_ATTR wifi_scan_done(void *arg, STATUS status) {

	if(status == OK) {
		struct bss_info *bss_link = (struct bss_info *)arg;
		if (bss_link) {
			os_timer_disarm(&staTimer);
			startWiFiSTA();
		}
	}
}


LOCAL void ICACHE_FLASH_ATTR staTimer_cb() {

	struct scan_config scanSt;
	int i;

	os_timer_disarm(&staTimer);

	if (!staApModeNow) {
		startWiFiSTA_AP();
	}

	if (wifi_get_opmode() == SOFTAP_MODE) {
		os_printf("AP mode can't scan!!!\n");
		os_timer_arm(&staTimer, 5000, false);
		return;
	}

	os_bzero(&scanSt, sizeof(struct scan_config));
	scanSt.ssid = wmConfig.staSsid;


/*	os_printf("sleepNow: %d\n", sleepNow);
	os_printf("powerLow: %d\n", powerLow);
	os_printf("wmConfig.apMode: %d\n", wmConfig.apMode);
	os_printf("staConfigure: %d\n", staConfigure);
	os_printf("apModeNow: %d\n", staModeNow);
	os_printf("wifi_station_get_connect_status(): %d\n", wifi_station_get_connect_status());
	os_printf("staModeNow: %d\n", staModeNow);*/


	if (!sleepNow && !powerLow && !wmConfig.apMode && staConfigure &&
			((apModeNow || (staModeNow && wifi_station_get_connect_status() != STATION_GOT_IP)) ||
																	(!apModeNow && !staModeNow))) {
	    INFO("Check WiFi network: %s\n", wmConfig.staSsid);
	    wifi_station_scan(&scanSt, wifi_scan_done);
	}
	os_timer_arm(&staTimer, 30000, false);
}

LOCAL void ICACHE_FLASH_ATTR wifi_event_cb(System_Event_t *evt) {

	char* mac;
	static bool timerOn = false;

	switch (evt->event) {
	case EVENT_STAMODE_CONNECTED:
		timerOn = false;
		INFO("Connected to %s!\n", wmConfig.staSsid);
		break;
	case EVENT_STAMODE_GOT_IP:
		timerOn = false;
		INFO("IP  address: %s\n", getIP(STATION_IF));
		mac = getMacAddress(STATION_IF);
		INFO("MAC address: %C%C:%C%C:%C%C:%C%C:%C%C:%C%C\n", mac[0],
				mac[1], mac[2], mac[3], mac[4], mac[5], mac[6], mac[7], mac[8],
				mac[9], mac[10], mac[11]);
		startSNTP();
	    mqttConnect();
		sleepTimerOn();
		break;
	case EVENT_STAMODE_DISCONNECTED:
	    if (!timerOn) {
	    	timerOn = true;
		    mqttDisconnect();
			os_timer_disarm(&staTimer);
			os_timer_setfn(&staTimer, (os_timer_func_t *)staTimer_cb, (void *)0);
			os_timer_arm(&staTimer, 5000, false);
	    }
		break;
	default:
		break;
	}
}


void ICACHE_FLASH_ATTR startWiFiSTA() {

	char *mac, *hostName;

	struct station_config sta_config;

	INFO("Start WiFi STA Mode\n");
	INFO("Connecting to: %s \n", wmConfig.staSsid);

	wifi_station_disconnect();
	wifi_station_dhcpc_stop();

	wifi_set_phy_mode(PHY_MODE_11G);

	wifi_station_get_config(&sta_config);

	wifi_set_opmode_current(STATION_MODE);

	mac = (char*)getMacAddress(STATION_IF);

	hostName = (char*)os_malloc(strlen(wmConfig.apSsid)+strlen(mac)+3);

	if (!hostName) {
		INFO("Error allocation memory in %lu line in %s file\n", __LINE__, __FILE__);
		return;
	}

	os_timer_disarm(&staTimer);

	staConfigure = true;

	os_sprintf(hostName, "%s-%s", wmConfig.apSsid, mac);

	if (strlen(hostName) > 32)
		hostName[32] = 0;

	wifi_station_set_hostname(hostName);

	os_free(hostName);

	sta_config.bssid_set = 0;

	os_sprintf(sta_config.ssid, "%s", wmConfig.staSsid);

	os_sprintf(sta_config.password, "%s", wmConfig.staPassword);

//	wifi_station_set_reconnect_policy(false);
	wifi_station_set_config_current(&sta_config);
	wifi_station_connect();
	wifi_station_dhcpc_start();
	wifi_set_event_handler_cb(wifi_event_cb);

	restartWiFi = false;
	apModeNow = false;
	staApModeNow = false;
	staModeNow = true;

	os_delay_us(50000);
	sleepTimerOn();
}


void ICACHE_FLASH_ATTR setNullWiFiConfigDefault() {

	struct softap_config ap_config;
	struct station_config sta_config;

	wifi_station_disconnect();

	wifi_softap_get_config_default(&ap_config);
	wifi_station_get_config_default(&sta_config);

	if (ap_config.ssid[0] != 0 || ap_config.password[0] != 0) {
		os_memset(&ap_config, 0, sizeof(struct softap_config));
		wifi_softap_set_config(&ap_config);
	}

	if (sta_config.ssid[0] != 0 || sta_config.password[0] != 0) {
		os_memset(&sta_config, 0, sizeof(struct station_config));
		wifi_station_set_config(&sta_config);
	}

}

