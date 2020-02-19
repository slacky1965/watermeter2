#ifndef INCLUDE_WM_WEB_H_
#define INCLUDE_WM_WEB_H_

#include "httpd.h"

#define INDEX_TPL "/index.tpl"

int ICACHE_FLASH_ATTR tplToken(HttpdConnData *connData, char *token, void **arg);
int ICACHE_FLASH_ATTR myCgiEspFsTemplate(HttpdConnData *connData);
int ICACHE_FLASH_ATTR myCgiUpdate(HttpdConnData *connData);
int ICACHE_FLASH_ATTR htmlSettings(HttpdConnData *connData);

#endif /* INCLUDE_WM_WEB_H_ */
