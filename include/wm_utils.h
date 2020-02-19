#ifndef INCLUDE_WM_UTILS_H_
#define INCLUDE_WM_UTILS_H_

#include "user_config.h"


#ifndef OTA_FLASH_MAP
#define OTA_FLASH_MAP 6
#endif

#if OTA_FLASH_MAP == 0
#define INITDATAPOS 0x7c
#endif
#if OTA_FLASH_MAP == 2
#define INITDATAPOS 0xfc
#endif
#if OTA_FLASH_MAP == 3 || OTA_FLASH_MAP == 5
#define INITDATAPOS 0x1fc
#endif
#if OTA_FLASH_MAP == 4 || OTA_FLASH_MAP == 6
#define INITDATAPOS 0x3fc
#endif

extern void ICACHE_FLASH_ATTR sleepTimerOn();

void ICACHE_FLASH_ATTR startSNTP();
void ICACHE_FLASH_ATTR setTimeStart(uint32_t ts);
uint32_t ICACHE_FLASH_ATTR getTimeStart();
uint32_t ICACHE_FLASH_ATTR localTimeT();
char* ICACHE_FLASH_ATTR localUpTime();
char* ICACHE_FLASH_ATTR localTimeStr();
char* ICACHE_FLASH_ATTR getMacAddress(uint8	if_index);
char* ICACHE_FLASH_ATTR getVoltage();
char* ICACHE_FLASH_ATTR getRssi();
char* ICACHE_FLASH_ATTR getIP(uint8_t mode);
void ICACHE_FLASH_ATTR startApMsg();
void ICACHE_FLASH_ATTR sleepOnNow();
void ICACHE_FLASH_ATTR displayUpdate();

#endif /* INCLUDE_WM_UTILS_H_ */
