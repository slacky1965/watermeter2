#define spi_CS_low		gpio_output_set(0,BIT15, BIT15, 0);  // cs LOW OUT
#define spi_CS_high     gpio_output_set( BIT15,0, BIT15, 0);  // cs high OUT

#include "driver/spi.h"
#include "driver/gpio16.h"
//#define DELAY 3000 /* milliseconds */
extern uint8 spi_no;
//#define GPCD   2  //DRIVER 0:normal,1:open drain
int media_init(void);





void spi_init(uint8 spi_no){
	
	spi_init_gpio(spi_no, SPI_CLK_USE_DIV);
	spi_clock(spi_no, SPI_CLK_PREDIV_LOW, SPI_CLK_CNTDIV_LOW);  // set spi freq to low  untill sd card init
	spi_tx_byte_order(spi_no, SPI_BYTE_ORDER_HIGH_TO_LOW);
	spi_rx_byte_order(spi_no, SPI_BYTE_ORDER_HIGH_TO_LOW); 

	//SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_CS_SETUP|SPI_CS_HOLD);
	CLEAR_PERI_REG_MASK(SPI_USER(spi_no), SPI_FLASH_MODE);

}

void spi_init_gpio(uint8 spi_no, uint8 sysclk_as_spiclk)
{
	uint32 clock_div_flag = 0;


	WRITE_PERI_REG(PERIPHS_IO_MUX, 0x105 | (clock_div_flag << 9)); //Set bit 9 if 80MHz sysclock required
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, 2); //GPIO12 becomes HSPI MISO pin (Master Data In) when set with 2
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, 2); //GPIO13 becomes HSPI MOSI pin (Master Data Out) when set to 2
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, 2); //GPIO14 becomes HSPI CLK pin (Clock) when set to 2
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, 3); //GPIO15 setting to 3 which makes pin GPIO15 we will control CS FROM WITHIN THE PROGRAM rather then via spi

}



//void sd_card_detect(void)
//{
//	uint8 direction = 0;
//	uint32 inputs;
//	os_delay_us(300);// wait a little for bounce
//
//	//Not sure if a read is required here but did it anyway
//	uint32 gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
//
//	//disable interrupt
//	gpio_pin_intr_state_set(GPIO_ID_PIN(5), GPIO_PIN_INTR_DISABLE);
//
//	//clear interrupt status
//	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status & BIT(5));
//	os_printf("in interrupt routine\n\n\n");
//
//
//	gpio_pin_intr_state_set(GPIO_ID_PIN(5), GPIO_PIN_INTR_ANYEDGE); //setting gpio5 to interrupt on any edge
//
//
//	//if (GPIO_INPUT_GET(5))
//	//{
//	//	//no sd return
//	//	return;
//
//	//}
//	// we have an sd card so set timer to init card
//
//	os_timer_disarm(&info_timer);
//	os_timer_setfn(&info_timer, (os_timer_func_t *)media_init, (void *)0);// set to goto media init function
//	os_timer_arm(&info_timer, DELAY, 0);// set for just one trigger
//
//
//
//
//}
//


void spi_clock(uint8 spi_no, uint16 prediv, uint8 cntdiv){
	
	if(spi_no > 1) return;

	if((prediv==0)|(cntdiv==0)){

		WRITE_PERI_REG(SPI_CLOCK(spi_no), SPI_CLK_EQU_SYSCLK);

	} else {
	
		WRITE_PERI_REG(SPI_CLOCK(spi_no),(((prediv-1)&SPI_CLKDIV_PRE)<<SPI_CLKDIV_PRE_S)|
					(((cntdiv-1)&SPI_CLKCNT_N)<<SPI_CLKCNT_N_S)|
					(((cntdiv>>1)&SPI_CLKCNT_H)<<SPI_CLKCNT_H_S)|
					((0&SPI_CLKCNT_L)<<SPI_CLKCNT_L_S));
	}

}


void spi_tx_byte_order(uint8 spi_no, uint8 byte_order){

	if(spi_no > 1) return;

	if(byte_order){
		SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_WR_BYTE_ORDER);
	} else {
		CLEAR_PERI_REG_MASK(SPI_USER(spi_no), SPI_WR_BYTE_ORDER);
	}
}


void spi_rx_byte_order(uint8 spi_no, uint8 byte_order){

	if(spi_no > 1) return;

	if(byte_order){
		SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_RD_BYTE_ORDER);
	} else {
		CLEAR_PERI_REG_MASK(SPI_USER(spi_no), SPI_RD_BYTE_ORDER);
	}
}






uint32 spi_rec(void)
{
	int k = 0;
	while (spi_busy(1)); //wait for SPI to be ready	


	
	//########## Enable SPI Functions ##########//
	//disable MOSI, MISO, ADDR, COMMAND, DUMMY in case previously set.
	CLEAR_PERI_REG_MASK(SPI_USER(spi_no), SPI_USR_MOSI | SPI_USR_MISO | SPI_USR_COMMAND | SPI_USR_ADDR | SPI_USR_DUMMY);
	
	
	SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_USR_MISO);  //set for data in
		
	WRITE_PERI_REG(SPI_USER1(1),((7)&SPI_USR_MISO_BITLEN) << SPI_USR_MISO_BITLEN_S ); //number of in bits
	
	
	
	SET_PERI_REG_MASK(SPI_CMD(spi_no), SPI_USR);//########## Begin SPI Transaction ##########//
	

	while (spi_busy(spi_no));	//wait for SPI transaction to complete
	
	return READ_PERI_REG(SPI_W0(spi_no)) >> (32 - 8); //Assuming data in is written to MSB. TBC
		
		
	
}

uint32 spi_send(uint8 spi_no,uint32 data)
{
	int k = 0;
	while (spi_busy(spi_no)); //wait for SPI to be ready	


	
	//########## Enable SPI Functions ##########//
	//disable MOSI, MISO, ADDR, COMMAND, DUMMY in case previously set.
	CLEAR_PERI_REG_MASK(SPI_USER(spi_no), SPI_USR_MOSI | SPI_USR_MISO | SPI_USR_COMMAND | SPI_USR_ADDR | SPI_USR_DUMMY);

	//########## Setup Bitlengths ##########//
	WRITE_PERI_REG(SPI_USER1(spi_no), ((7)&SPI_USR_MOSI_BITLEN) << SPI_USR_MOSI_BITLEN_S); //Number of bits to Send
	
	//########## Setup DOUT data ##########//
	
		SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_USR_MOSI); //enable MOSI function in SPI module
		//copy data to W0
	//	if (READ_PERI_REG(SPI_USER(spi_no))&SPI_WR_BYTE_ORDER) 
	//	{
			WRITE_PERI_REG(SPI_W0(spi_no), data << (32 - 8));
	//	}
			WRITE_PERI_REG(SPI_W0(spi_no), data << (32 - 8));
	
	//########## END SECTION ##########//
	//	os_printf("user reg\n%x", READ_PERI_REG(SPI_USER(1)));
	//########## Begin SPI Transaction ##########//
	SET_PERI_REG_MASK(SPI_CMD(spi_no), SPI_USR);
	//########## END SECTION ##########//

	


	//os_printf("%x\n\r", data);













	return 1; //success
}
void ICACHE_FLASH_ATTR print_sector(uint32 sector, uint8 *data)
{
	int i;
	int h=1;
	int g;

	os_printf("SECTOR NUMBER =   %x\n\n", sector);

	for (g =0; g < 512; g++)
	{

		os_printf("    %x-   %x   -%d    ", g, data[g], g);


		if (h == 4)
		{
			os_printf("\n\n");
			h = 0;
		}
		h++;

	}

}
