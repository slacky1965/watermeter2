#include "eagle_soc.h"
#include "driver/spi.h"
#include "fat_filelib.h"
#include "Sd2Card.h"
#include "fat_opts.h"
#include "sdinfo.h"
#include "fat_access.h"

#include "global.h"
#include "wm_utils.h"


#define CMD00 0
#define CMD00R 1
#define CMD55 55
#define CMD55R 1
#define CMD01 01
#define CMD01R 1
#define CMD10 10
#define CMD10R 16
#define CMD58 58
#define CMD15 15
#define CMD58R 5
#define ACMD41 41
#define CMD17 17
#define CMD17R 512
#define CMD24 24
#define CMD24R 1
#define ACMD41 41
#define ACMD41R 1
#define CMD8R 5

#define spi_CS_low		gpio_output_set(0,BIT15, BIT15, 0);  // clk LOW OUT
#define spi_CS_high     gpio_output_set( BIT15,0, BIT15, 0);  // clk high OUT


uint8 spi_no = 1;
static unsigned char response[580];
uint32_t arg;

//LOCAL os_timer_t info_timer;
//static os_timer_t delay_timer;
//static os_timer_t micros_overflow_timer;
static uint32_t micros_at_last_overflow_tick = 0;
static uint32_t micros_overflow_count = 0;




uint8  ICACHE_FLASH_ATTR SendSdCardCommandAndWriteBlock(uint8 cmd, int32 commandBytes, uint8 CRC, unsigned char *response, int responseLength, unsigned char *dataBlock) {
	spi_CS_low;
	uint8 data = 0;
	int s;
	int i;
	spi_send(1, cmd);
	for (s = 24; s >= 0; s -= 8)
	{
		data = commandBytes >> s & 0xff;
		spi_send(1, data);
	}
	//os_printf("sector written ==%x \n\r", commandBytes);



	spi_send(1, CRC);
	os_delay_us(500);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, 3); //GPIO13 is HSPI MOSI pin (Master Data Out)
	gpio_output_set(BIT13, 0, BIT13, 0);  //MOSI HIGH OUT



	// wait for response
	for (i = 0; ((status_ = spi_rec(1)) & 0x80) && i != 0xFF; i++);
	//os_printf("SendSdCardCommandAndWriteBlock response == %x\n", status_);
	if (i == 0xff)
	{


		os_printf("in SendSdCardCommandAndWriteBlock no response\n\r");
		goto fail;
	}
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, 2); //GPIO13 is HSPI MOSI pin (Master Data Out)

	spi_send(1, 0xFE); //send start data token

	for ( i = 0; i < 512; i++)
	{
		spi_send(1, dataBlock[i]);
	}

	spi_send(1,0xff);  // dummy crc
	spi_send(1,0xff);  // dummy crc

	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, 3); //GPIO13 is HSPI MOSI pin (Master Data Out)
	gpio_output_set(BIT13, 0, BIT13, 0);  //MOSI HIGH OUT
	os_delay_us(300);
	status_ = spi_rec(1);




	if ((status_ & DATA_RES_MASK) != DATA_RES_ACCEPTED)
	{


			//print_sector(commandBytes,dataBlock);

		os_printf("in SendSdCardCommandAndWriteBlock data not accepted\n\r");
		waitNotBusy(SD_WRITE_TIMEOUT);
		os_printf("status received ==%x\n\r",status_);

		error(SD_CARD_ERROR_WRITE);
		spi_CS_high;
		spi_send(1, 0xff);
		spi_send(1, 0xff);
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, 2); //GPIO13 is HSPI MOSI pin (Master Data Out)
		return false;
	}
	// wait for flash programming to complete
	if (!waitNotBusy(SD_WRITE_TIMEOUT)) {
		error(SD_CARD_ERROR_WRITE_TIMEOUT);
		goto fail;
	}
	spi_CS_high;
	spi_send(1, 0xff);
	spi_send(1, 0xff);
	return data;
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, 2); //GPIO13 is HSPI MOSI pin (Master Data Out)
fail:
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, 2); //GPIO13 is HSPI MOSI pin (Master Data Out)
	spi_CS_high;
//	os_timer_disarm(&info_timer);
	return false;
}



void  ICACHE_FLASH_ATTR GetBlockReadResponse(unsigned char *response, int responseLength)
{
	//os_printf("in block read command\n");
	int g = 0;
	int i = 0;
	int x= 0;
	uint8 y;
	uint8_t SdCardResponse = 0;
	int temp;
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, 3); //GPIO13 is HSPI MOSI pin (Master Data Out)
	gpio_output_set(BIT13, 0, BIT13, 0);  //MOSI HIGH OUT
	os_delay_us(500);


	for (i = 0; ((response[0] = spi_rec(1)) & 0x80) && i != 0xFF; i++);

	if (i == 0xFF)
	{
		os_printf("spiRec Timed out\n\r");

	}
	for (i = 0; (temp = spi_rec(1)) != 0xFE && i != 0xFFFF; i++);

	if (i == 0xFFFF)
	{
		os_printf("Response token was never received\n\r");
		response[0] = 0xFF;
	}
	else
	{
		for (i = 0; i < responseLength ; i++)
		{
			response[i] = spi_rec(1);  //spi_rec(1);
		}
		temp = spi_rec(1);  // discard crc
		temp = spi_rec(1);
	}
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, 2); //GPIO13 is HSPI MOSI pin (Master Data Out)
	os_delay_us(1000);
}



void  ICACHE_FLASH_ATTR GetCommandResponse(unsigned char *response, int responseLength) {
	int i = 0;
	uint8_t SdCardResponse = 0;
	while (spi_busy(spi_no))
		; //wait for SPI to be ready
	os_delay_us(100);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, 3); //GPIO13 is HSPI MOSI pin (Master Data Out)
	gpio_output_set(BIT13, 0, BIT13, 0);  //MOSI HIGH OUT

	for (i = 0; ((response[0] = spi_rec(1)) & 0x80) && i != 0xFF; i++)
		;
	//os_printf("Command Response = %x", response[0]);
	if (i == 0xFF) {
		os_printf("spiRec Timed out\n\r");
		response[0] = 0xFF;
	}
	for (i = 1; i < responseLength + 1; i++) {
		response[i] = spi_rec(1); //spi_rec(1);
		//os_printf("CMD8 RESPONSE = %x\n", response[i]);
	}
	spi_CS_high
	;
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, 2); //GPIO13 is HSPI MOSI pin (Master Data Out)
	os_delay_us(5000);
}




uint8  ICACHE_FLASH_ATTR SendSdCardCommand(uint8 cmd, int32 commandBytes, uint8 CRC, unsigned char *response, int responseLength)
{
	spi_CS_low;
	uint8 data = 0;
	int s;
	spi_send(1, cmd);
	for (s = 24; s >= 0; s -= 8)
	{
		data = commandBytes >> s & 0xff;
		spi_send(1, data);
	}
	spi_send(1, CRC);
	if (cmd == (CMD17 | 0x40))
	{
		GetBlockReadResponse(response, responseLength);
	}
	else
	{
		GetCommandResponse(response, responseLength);
	}
	spi_CS_high;
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, 2); //GPIO13 is HSPI MOSI pin (Master Data Out)
	spi_send(1, 0xff);
	spi_send(1, 0xff);
	return data;
}

unsigned long  ICACHE_FLASH_ATTR milliss() {
	uint32_t m = system_get_time();
	uint32_t c = micros_overflow_count + ((m < micros_at_last_overflow_tick) ? 1 : 0);
	return c * 4294967 + m / 1000;
}


uint8_t  ICACHE_FLASH_ATTR waitNotBusy(uint16_t timeoutMillis) {
	uint16_t t0 = (uint16_t)milliss();
	do
	{
		//os_printf("in wait busy value = %x\n\r", spi_rec(1));
		if (spi_rec(1) == 0XFF) return true;
	} while (((uint16_t)milliss() - t0) < timeoutMillis);
	return false;
}


int  ICACHE_FLASH_ATTR media_read(unsigned long sector, unsigned char *buffer,
		unsigned long sector_count) {

	unsigned long i;
	unsigned long sector1;
	//os_printf("in media read sector read1 == %x count == %x\n\r", sector, sector_count);
	for (i = 0; i < sector_count; i++) {

		//os_printf("in media read sector read2 == %x count == %x\n\r", sector, sector_count);
		sector1 = sector;

		if (type_ == SD_CARD_TYPE_SD2) {
			os_printf("sd card type sd2 ???\n");
			sector1 <<= 9;

		}
		SendSdCardCommand(CMD17 | 0x40, sector1, 0xff, buffer, CMD17R);
		sector++;
		buffer += 512;
	}

	return 1;
}

int  ICACHE_FLASH_ATTR media_write(unsigned long sector, unsigned char *buffer,
		unsigned long sector_count) {
	//ets_intr_lock();
	unsigned long i;
	unsigned long sector1;

	for (i = 0; i < sector_count; i++) {

		//os_printf("in media write sector written == %x\n\r", sector);
		sector1 = sector;
		if (type_ == SD_CARD_TYPE_SD2) {

			//os_printf("in media write sector count ==%x\n\r", sector_count);
			sector1 <<= 9;

		}
		SendSdCardCommandAndWriteBlock(CMD24 | 0x40, sector1, 0xff, response,
				CMD24R, buffer);
		spi_CS_high
		;
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, 2); //GPIO13 is HSPI MOSI pin (Master Data Out)
		sector++;
		buffer += 512;
	}
	//ets_intr_unlock();
	return 1;
}


void  ICACHE_FLASH_ATTR InitFat()
{
	fl_init();
	//Attach media access functions to library
	if (fl_attach_media() != FAT_INIT_OK)
	{
		os_printf("ERROR: Media attach failed\n");
		sd_ok = false;
		return;
	}
	sd_ok = true;
}


int ICACHE_FLASH_ATTR media_init() {
	//if (GPIO_INPUT_GET(5))  // see if sd card present if not return
	//{
	//	return;

	//}
	os_delay_us(5000);
	spi_send(1, 0xff); // send sd card 74 or more clock cycles per card requirement
	spi_send(1, 0xff);
	spi_send(1, 0xff);
	spi_send(1, 0xff);
	spi_send(1, 0xff);
	spi_send(1, 0xff);
	spi_send(1, 0xff);
	spi_send(1, 0xff);
	spi_send(1, 0xff);
	spi_send(1, 0xff);
	spi_send(1, 0xff);
	spi_send(1, 0xff);
	os_delay_us(500);
	int i;
	int counter = 0;
	static unsigned char argument[4];
	SendSdCardCommand(CMD00 | 0x40, 0x00000000, 0x95, response, CMD00R);
	if (response[0] == 1) {
		os_printf("CMD0 Successful\n");
	} else {
		os_printf("CMD0 Failed with response of %x\n", response[0]);
		goto fail;
	}
	spi_send(1, 0xff);
	spi_send(1, 0xff);
	os_delay_us(1500);

	SendSdCardCommand(CMD8 | 0x40, 0x000001AA, 0x87, response, CMD8R);
	if (response[0] == R1_ILLEGAL_COMMAND) {
		type_ = SD_CARD_TYPE_SD1;
	} else if (response[4] != 0xAA) {
		os_printf("CMD8 failed with status %x\n", status_);
		goto fail;
	}
	os_printf("CMD8 Successful\n");
	type_ = SD_CARD_TYPE_SD2;
	int count = 0;
	arg = type() == SD_CARD_TYPE_SD2 ? 0X40000000 : 0;
	while (response[0] != 00 && count < SD_INIT_TIMEOUT) {

		SendSdCardCommand(CMD55 | 0x40, 0x00000000, 0x87, response, CMD8R);
		SendSdCardCommand(ACMD41 | 0x40, arg, 0xff, response, ACMD41R);

		count++;
	}
	if (count >= SD_INIT_TIMEOUT) {
		os_printf("ACMD41  fail due to timeout\n ");
	} else if (type() == SD_CARD_TYPE_SD2) {

		SendSdCardCommand(CMD58 | 0x40, 0x00000000, 0xff, response, CMD58R);
		if (response[0] != 0)
			os_printf("CMD58  failed with response of %x\n ", response[0]);
		if (response[1] == 0xC0)
			type_ = SD_CARD_TYPE_SDHC;
		os_printf("type card = %d\n", type_);
	}

	spi_clock(spi_no, SPI_CLK_PREDIV_HIGH, SPI_CLK_CNTDIV_HIGH); //set spi clock to high  20mhz

	InitFat();
	fail: spi_CS_high;
	spi_send(1, 0xff);
	spi_send(1, 0xff);

}


bool ICACHE_FLASH_ATTR sd_init() {

  os_printf("Initialising SD card ...\r\n");

  media_init();

  if (!sd_ok) {
    os_printf("SD initialising False.\r\n");
  } else {
    os_printf("SD initialising OK.\r\n");
  }
  return sd_ok;
}

bool ICACHE_FLASH_ATTR checkDir(char* dir) {
	FL_DIR fdir;

	if (!fl_opendir(dir, &fdir)) {
		os_printf("Directory: %s not found\r\n", dir);
		return false;
	}

	fl_closedir(&fdir);

	return true;
}

bool ICACHE_FLASH_ATTR mkDir(char* dir) {

	if (!sd_ok) return false;

	if (!checkDir(dir)) {
		return fl_createdirectory(dir);
	}

	return true;
}


