#include "esp8266.h"
#include "user_interface.h"
#include "spi_flash.h"
#include "driver/i2c_master.h"

#include "global.h"
//#include "user_config.h"
#include "ssd1306.h"

#include "wm_utils.h"
#include "wm_wifi.h"
//#include "mqtt.h"


LOCAL os_timer_t sntpTimer;
static char buff[18] = { 0 };
uint32_t timeStart;

void ICACHE_FLASH_ATTR displayUpdate() {

	char buf[11];

	displayWrite = false;

	display_clear();

	gfx_setTextSize(2);

	gfx_setTextColor(WHITE);

	gfx_setCursor(0, 0);

	os_sprintf(buf, "Hot  %5u", wmConfig.hotWater/1000);

	gfx_print(buf);

	os_sprintf(buf, "Cold %5u", wmConfig.coldWater/1000);

	gfx_print(buf);

	display_update();

}

void ICACHE_FLASH_ATTR set107init_data_bin() {
	char *buf;
	SpiFlashOpResult ret;

	buf = (char*) os_malloc(SPI_FLASH_SEC_SIZE);

	if (!buf) {
		INFO("Error allocation memory in %lu line in %s file\n", __LINE__, __FILE__);
		return;
	}

	ret = spi_flash_read(INITDATAPOS * SPI_FLASH_SEC_SIZE, (uint32*) buf,
			SPI_FLASH_SEC_SIZE);
	if (ret == SPI_FLASH_RESULT_OK) {

		if (buf[107] != ADC_TOUT) {

			buf[107] = ADC_TOUT;

			ets_intr_lock();

			spi_flash_erase_sector(INITDATAPOS);

			spi_flash_write(INITDATAPOS * SPI_FLASH_SEC_SIZE, (uint32*) buf,
					SPI_FLASH_SEC_SIZE);

			ets_intr_unlock();

			system_restart();

			os_delay_us(50000);
		}
	}

	os_free(buf);
}

/* Battery in volt */
char* ICACHE_FLASH_ATTR getVoltage() {
	uint16 vcc;

	vcc = system_adc_read();

	os_sprintf(buff, "Battery: %d,%dV", (4200 / 1024 * vcc) / 1000,
			((4200 / 1024 * vcc) % 1000) / 10);

	return buff;
}

/* Received signal strength indicator in dBm */
char* ICACHE_FLASH_ATTR getRssi() {

	os_sprintf(buff, "WiFi: %d dBm", wifi_station_get_rssi());

	return buff;
}

LOCAL void ICACHE_FLASH_ATTR wakeupFromMotion(void) {
	wifi_fpm_close();
	INFO("Wake up from sleep.\n");
	sleepNow = false;
	dontSleep = true;
/*	display_init(I2C_ADDR);
	os_delay_us(5000);
	display_clear();
	display_update();*/
}

void ICACHE_FLASH_ATTR sleepOnNow() {
/*	display_off(I2C_ADDR);
	os_delay_us(5000);*/
	mqttDisconnect();
	apModeNow = false;
	staModeNow = false;
	staApModeNow = false;
	wifi_station_disconnect();
	wifi_set_opmode_current(NULL_MODE);
//	wifi_set_opmode(NULL_MODE);
	if (wifi_get_opmode() != NULL_MODE)
		return;
//	ETS_GPIO_INTR_ENABLE();
	os_delay_us(50000);
	wifi_fpm_set_sleep_type(LIGHT_SLEEP_T); //light sleep mode
	gpio_pin_wakeup_enable(GPIO_ID_PIN(pin_num[HOT_PIN]),
			GPIO_PIN_INTR_LOLEVEL); /* Set the interrupt to look for LOW pulses on HOT_PIN  */
	gpio_pin_wakeup_enable(GPIO_ID_PIN(pin_num[COLD_PIN]),
			GPIO_PIN_INTR_LOLEVEL); /* Set the interrupt to look for LOW pulses on COLD_PIN */
	wifi_fpm_open();
	wifi_fpm_set_wakeup_cb(wakeupFromMotion); //wakeup callback
	wifi_fpm_do_sleep(0xFFFFFFF);
	system_soft_wdt_feed();
	os_delay_us(50000);
}

char* ICACHE_FLASH_ATTR getIP(uint8_t mode) {
	struct ip_info ip;
	wifi_get_ip_info(mode, &ip);
	os_sprintf(buff, "%d.%d.%d.%d", ip.ip.addr & 0xFF, (ip.ip.addr >> 8) & 0xFF,
			(ip.ip.addr >> 16) & 0xFF, (ip.ip.addr >> 24) & 0xFF);

	return buff;
}

void ICACHE_FLASH_ATTR startApMsg() {
	INFO("WiFi network Name: %s, Password: %s\n", wmConfig.apSsid,
			wmConfig.apPassword);
	INFO("Go to: %s please\n", getIP(SOFTAP_IF));
}

char* ICACHE_FLASH_ATTR getMacAddress(uint8 if_index) {
	uint8_t mac[6];
	wifi_get_macaddr(if_index, mac);
//	os_sprintf(buff, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	os_sprintf(buff, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3],
			mac[4], mac[5]);

	return buff;
}

void ICACHE_FLASH_ATTR setTimeStart(uint32_t ts) {
	timeStart = ts;
}

uint32_t ICACHE_FLASH_ATTR getTimeStart() {
	return timeStart;
}

char* ICACHE_FLASH_ATTR localUpTime() {

	char tmp[6] = { 0 };
	uint32_t milli;

	if (firstNTP) {
		milli = milliss();
	} else {
		milli = (sntp_get_current_timestamp() - timeStart) * 1000;
	}

	uint32_t secs = milli / 1000, mins = secs / 60;
	uint16_t hours = mins / 60, days = hours / 24;

	milli -= secs * 1000;
	secs -= mins * 60;
	mins -= hours * 60;
	hours -= days * 24;

	if (days) {
		os_sprintf(tmp, "%ud", days);
	}

	os_sprintf(buff, "%s %02u:%02u:%02u", tmp[0] == 0 ? "" : tmp, hours, mins,
			secs);

	return buff;
}

uint32_t ICACHE_FLASH_ATTR localTimeT() {

	uint32_t t =
			sntp_get_current_timestamp() + wmConfig.timeZone * SECS_PER_HOUR;

	return t;
}

char* ICACHE_FLASH_ATTR localTimeStr() {

	char *currentTime;

	uint32_t t = localTimeT();

	currentTime = (char*) sntp_get_real_time(t);

	return currentTime;
}

LOCAL void ICACHE_FLASH_ATTR check_sntp_stamp_cb() {

	uint32 current_stamp;

	current_stamp = sntp_get_current_timestamp();

	if (current_stamp == 0) {

		sntp_stop();

		if ( true == sntp_set_timezone(0)) {

			sntp_init();

		}
		os_timer_disarm(&sntpTimer);
		os_timer_arm(&sntpTimer, 1000, false);

	} else {

		responseNTP = true;

		os_timer_disarm(&sntpTimer);
		os_printf("sntp: %s",
				sntp_get_real_time(
						current_stamp + wmConfig.timeZone * SECS_PER_HOUR));

	}

}

void ICACHE_FLASH_ATTR startSNTP() {

	os_printf("Start SNTP: %s\n", wmConfig.ntpServerName);

	os_timer_disarm(&sntpTimer);
	os_timer_setfn(&sntpTimer, (os_timer_func_t *) check_sntp_stamp_cb, NULL);
	os_timer_arm(&sntpTimer, 1000, false);

}

