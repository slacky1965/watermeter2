#ifndef INCLUDE_WM_CONFIG_H_
#define INCLUDE_WM_CONFIG_H_

/* Structure of config file */
typedef struct config {
  char webAdminLogin[16];      /* Login for web Auth                    */
  char webAdminPassword[16];   /* Password for web Auth                 */
  bool fullSecurity;           /* true - all web Auth, false - free     */
  bool configSecurity;         /* true - only config and update Auth    */
  char staSsid[16];            /* STA SSID WiFi                         */
  char staPassword[16];        /* STA Password WiFi                     */
  bool apMode;                 /* true - AP Mode, false - STA Mode      */
  char apSsid[16];             /* WiFi Name in AP mode                  */
  char apPassword[16];         /* WiFi Password in AP mode              */
  char mqttBroker[32];         /* URL or IP-address of mqtt-broker      */
  char mqttUser[16];           /* mqtt user                             */
  char mqttPassword[16];       /* mqtt password                         */
  char mqttTopic[64];          /* mqtt topic                            */
  char ntpServerName[32];      /* URL or IP-address of NTP server       */
  sint8 timeZone;              /* Time Zone                             */
  uint8 litersPerPulse;        /* liters per pulse                      */
  uint32 hotTime;              /* Last update time of hot water         */
  uint32 hotWater;             /* Last number of liters hot water       */
  uint32 coldTime;             /* Last update time of cold water        */
  uint32 coldWater;            /* Last number of litres cold water      */
} _config;

void ICACHE_FLASH_ATTR initDefConfig(_config* config);
void ICACHE_FLASH_ATTR setConfig(_config *config);
void ICACHE_FLASH_ATTR pritnfConfig(_config* config);
void ICACHE_FLASH_ATTR removeConfig();
void ICACHE_FLASH_ATTR clearConfig(_config *config);

#endif /* INCLUDE_WM_CONFIG_H_ */
