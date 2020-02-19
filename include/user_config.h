#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#include "os_type.h"
#include "driver/gpio16.h"

#include "wemos.h"
#include "wm_config.h"

#define DEBUG false

#if DEBUG == true
#define INFO os_printf
#else
#define INFO ets_uart_printf
#endif

#define	ADC_TOUT	36							  /* Constant for system_adc_read() 	  */

#define SLEEP_MODE_ON true                        /* To pass into sleep mode if true      */
#define NOT_READ_EEPROM false                     /* Dont't read from EEPROM if true      */
#define SLEEP_DELAY 2							  /* Delay in sec. before to sleep		  */

/* Name and Version */
#define PLATFORM "Wemos D1 mini & Micro SD"
#define MODULE_VERSION "v3.0"
#define MODULE_NAME "WaterMeter " MODULE_VERSION
#define WEB_WATERMETER_FIRST_NAME "Water"
#define WEB_WATERMETER_LAST_NAME "Meter"

/* Directory and name of config files for SD card */
#define DELIM "/"
#define WM_DIR DELIM "watermeter"                    /* Drirectory for config file        - "/watermeter"             	 */
#define WM_CONFIG_FILE WM_DIR DELIM "watermeter.dat" /* Full path and name of config file - "/watermeter/watermeter.dat" */

#define HTTPROOT DELIM

/* Define pin name */
#define HOT_PIN D1                                /* Pin of hot water                     */
#define COLD_PIN D2                               /* Pin of cold water                    */
#define EXT_POWER_PIN D0                          /* Pin of monitoring external power     */

/* For config (default settings) */
#define WEB_ADMIN_LOGIN "Admin"                  /* Login for web Auth                */
#define WEB_ADMIN_PASSWORD "1111"                /* Password for web Auth             */
#define AP_SSID "WaterMeter_" MODULE_VERSION     /* Name WiFi AP Mode                 */
#define AP_PASSWORD "12345678"                   /* Password WiFi AP Mode             */
#define MQTT_USER "test"                         /* mqtt user                         */
#define MQTT_PASSWORD "1111"                     /* mqtt password                     */
#define MQTT_BROKER "192.168.1.1"                /* URL mqtt broker                   */
#define MQTT_TOPIC "WaterMeter"                  /* Primary mqtt topic name           */
#define LITERS_PER_PULSE 10                      /* How many liters per one pulse     */
#define TIME_ZONE 3                              /* Default Time Zone                 */

/* For sntp */
//#define SYNC_TIME 21600                          /* Interval sync to NTP Server in sec           */
#define NTP_SERVER_NAME "ntp4.stratum2.ru"       /* URL of NTP server                            */
#define SECS_PER_HOUR 3600

/* For MQTT client server */
//#define MQTT_SSL_ENABLE
#define END_TOPIC_HOT_IN "HotWater" DELIM "In"
#define END_TOPIC_COLD_IN "ColdWater" DELIM "In"
#define END_TOPIC_HOT_OUT "HotWater" DELIM "Out"
#define END_TOPIC_COLD_OUT "ColdWater" DELIM "Out"
#define SIZE_END_TOPIC 32

#define MQTT_PORT     1883
#define MQTT_BUF_SIZE   1024
#define MQTT_KEEPALIVE    120  /*second*/
#define MQTT_CLEAN_SESSION 1
#define MQTT_RECONNECT_TIMEOUT  5 /*second*/

#define DEFAULT_SECURITY  0
#define QUEUE_BUFFER_SIZE       2048

#define PROTOCOL_NAMEv31  /*MQTT version 3.1 compatible with Mosquitto v0.15*/
//PROTOCOL_NAMEv311     /*MQTT version 3.11 compatible with https://eclipse.org/paho/clients/testing/*/

typedef struct {
  uint8_t status;
  uint8_t status_old;
  uint32_t counterHotWater;
  uint32_t counterColdWater;
} _waterCounter;

extern const char* watermeterDirName;
extern const char* configFileName;

extern uint8 spi_no;

extern uint8_t pin_num[GPIO_PIN_NUM];

extern int ets_uart_printf(const char *fmt, ...);
extern void disk_timerproc (void);



#endif
