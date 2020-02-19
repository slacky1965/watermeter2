#ifndef INCLUDE_GLOBAL_H_
#define INCLUDE_GLOBAL_H_

#include "user_config.h"

/* flags */
extern bool rebootNow;
extern bool restartWiFi;
extern bool sleepNow;
extern bool dontSleep;
extern bool powerLow;
extern bool apModeNow;
extern bool staModeNow;
extern bool staApModeNow;
extern bool staConfigure;
extern bool defaultConfig;
extern bool saveNewConfig;
extern bool firstStart;
extern bool responseNTP;
extern bool mqttRestart;
extern bool mqttFirstStart;
extern bool mqttConnected;
extern bool subsHotWater;
extern bool subsColdWater;
extern bool firstNTP;
extern bool sd_ok;
extern bool displayWrite;

extern _config wmConfig;
extern _waterCounter waterCounter;

#endif /* INCLUDE_GLOBAL_H_ */
