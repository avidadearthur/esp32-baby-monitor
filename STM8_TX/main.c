#include "stm8s.h"
#include "stm8s_spi.h"
#include "rf24l01.h"
#include "stm8s_it.h"


/*
RF24L01 connector pinout:
	GND    VCC
	CE     CSN
	SCK    MOSI
	MISO   IRQ
	
Connections:
  PC3 -> CE
  PC4 -> CSN
  PC7 -> MISO
  PC6 -> MOSI
  PC5 -> SCK
*/

uint8_t value[3];

float temp1 = 42.5;
float temp2 = 25.5;
float temp3 = 12.4;


uint8_t raddr[5] = {0x41, 0x42, 0x43, 0x44, 0x45};
uint8_t taddr[5] = {0x46, 0x47, 0x48, 0x49, 0x4A};

int counter1 = 0;
int counter2 = 0;
uint8_t checkVal;

uint16_t temp_1;
uint16_t temp_2;
uint16_t temp_3;

bool statusRT;


void delay_ms (int ms) //Function Definition 
	{
		int i = 0;
		int j = 0;
	
		for ( i=0; i<=ms; i++)
	
			for ( j=0; j<120; j++) // Nop = Fosc/4
	
				_asm("nop"); //Perform no operation //assembly code 
	
	}
	

main()
{
	RF24L01_init(); 
	
	statusRT = 0;
	raddr[0] = 0x41;
	raddr[1] = 0x42;
	raddr[2] = 0x43;
	raddr[3] = 0x44;
	raddr[4] = 0x45;
	
	taddr[0] = 0x41;
	taddr[1] = 0x42;
	taddr[2] = 0x43;
	taddr[3] = 0x44;
	taddr[4] = 0x45;
	
	GPIO_Init(GPIOA, GPIO_PIN_2, GPIO_MODE_OUT_PP_LOW_FAST); // LED2
  setup2(raddr, taddr);
	
	//NRF_MODE_RX(); 
	
	
	while(1)
	{
		NRF_MODE_TX();
		
		// convert temp to integer
		temp_1 = (int16_t)(temp1 * 10.0);
		
		// copy temp and status to values array
		value[0] = (uint8_t)(temp_1 & 0xFF); // lower byte
		value[1] = (uint8_t)((temp_1 >> 8) & 0xFF); // upper byte
		value[2] = 0;
		
		RF24L01_write_payload(value, 3);
		
		delay_ms(10000);
		
		// convert temp to integer
		temp_2 = (int16_t)(temp2 * 10.0);
		
		// copy temp and status to values array
		value[0] = (uint8_t)(temp_2 & 0xFF); // lower byte
		value[1] = (uint8_t)((temp_2 >> 8) & 0xFF); // upper byte
		value[2] = 1;
		
		RF24L01_write_payload(value, 3);
		
		delay_ms(10000);
		
		// convert temp to integer
		temp_3 = (int16_t)(temp3 * 10.0);
		
		// copy temp and status to values array
		value[0] = (uint8_t)(temp_3 & 0xFF); // lower byte
		value[1] = (uint8_t)((temp_3 >> 8) & 0xFF); // upper byte
		value[2] = 0;
		
		RF24L01_write_payload(value, 3);
		
		delay_ms(10000);
		
		//checkVal = RF24L01_read_register(0x0f);
	}

}


#ifdef USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param file: pointer to the source file name
  * @param line: assert_param error line source number
  * @retval : None
  */
void assert_failed(u8* file, u32 line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif
