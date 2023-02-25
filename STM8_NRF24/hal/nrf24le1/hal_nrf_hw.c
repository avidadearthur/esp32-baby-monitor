#include <stdint.h>
#include "hal_nrf.h"
#include "STM8l10x_conf.h"

uint8_t hal_nrf_rw(uint8_t value)
{
 while (RESET == SPI_GetFlagStatus(SPI_FLAG_TXE));
 SPI_SendData(value);
 
 //wait for receiving a byte
 while(RESET == SPI_GetFlagStatus(SPI_FLAG_RXNE));
 return SPI_ReceiveData();
}