#include "esp8266.h"
//#include "ets_sys.h"
//#include "osapi.h"
#include "os_type.h"
#include "user_interface.h"
#include "user_config.h"
#include "fat_filelib.h"
int time_status = 0;
#define IS_LEAP(year) (year%4 == 0)
#define SEC_IN_NON_LEAP (86400*365)
#define SEC_IN_LEAP (86400*366)
#define SEC_IN_YEAR(year) (IS_LEAP(year) ? SEC_IN_LEAP : SEC_IN_NON_LEAP)
int year,month,day, hour, min, sec;
//char buf[30];
void ICACHE_FLASH_ATTR epoch_to_str(unsigned long epoch);
unsigned char calendar[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
unsigned char calendar_leap[] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
uint32 EPOCH_time = 0;
//extern connState;

unsigned char ICACHE_FLASH_ATTR *get_calendar(int year) {
	return IS_LEAP(year) ? calendar_leap : calendar;
}

int ICACHE_FLASH_ATTR get_year(unsigned long *t) {
	int year = 1970;
	while (*t>SEC_IN_YEAR(year)) {
		*t -= SEC_IN_YEAR(year);
		year++;
	}
	return year;
}

int ICACHE_FLASH_ATTR get_month(unsigned long *t, int year) {
	unsigned char *cal = get_calendar(year);
	int i = 0;
	while (*t > cal[i] * 86400) {
		*t -= cal[i] * 86400;
		i++;
	}
	return i + 1;
}

void ICACHE_FLASH_ATTR epoch_to_str(unsigned long epoch)
{
	year = get_year(&epoch);
	month = get_month(&epoch, year);
	day = 1 + (epoch / 86400);
	epoch = epoch % 86400;
	hour = epoch / 3600;
	epoch %= 3600;
	min = epoch / 60;
	sec = (epoch % 60);

	//os_printf("time == %02d:%02d:%02d\n", hour, min, sec);
	//os_printf("%02d:%02d:%02d %02d/%02d/%02d\n\n", hour, min, sec, month, day, year);
	
}

void ICACHE_FLASH_ATTR  user_esp_platform_discon_cb(void *arg)
{
	os_printf("in disconnect\n");




}

void network_time_control(void)
{
	switch (time_status)
	{
		os_printf("switch =%x\n ", time_status);


	case 1:
		break;
	case 2:
		break;
	case 3:
		break;
	case 4:
		break;
	case 5:
		break;
	case 6:
		break;
	case 7:
		break;
	default:
		
		break;
	}

}
