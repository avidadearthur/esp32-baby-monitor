#ifndef HAL_NRF_STM8L101XX_H__
#define HAL_NRF_STM8L101XX_H__
#include "STM8l10x_conf.h"

#define data     
#define pdata
#define xdata

/** Macro that set radio's CSN line LOW.
 *
 */
#define CSN_LOW() do {  GPIO_ResetBits( GPIOB, GPIO_Pin_4);} while(false)
/** Macro that set radio's CSN line HIGH.
 *
 */
#define CSN_HIGH() do { GPIO_SetBits( GPIOB, GPIO_Pin_4);} while(false)

/** Macro that set radio's CE line LOW.
 *
 */
#define CE_LOW() do {GPIO_ResetBits(GPIOB, GPIO_Pin_3);} while(false)

/** Macro that set radio's CE line HIGH.
 *
 */
#define CE_HIGH() do {GPIO_SetBits(GPIOB, GPIO_Pin_3);} while(false)

/** Macro for writing the radio SPI data register.
 *
 */
#define HAL_NRF_HW_SPI_WRITE(d) do{\
                 while (SPI_GetFlagStatus(SPI_FLAG_TXE) == RESET);\
                 SPI_SendData(d);\
                } while(false)

/** Macro for reading the radio SPI data register.
 *
 */
#define HAL_NRF_HW_SPI_READ()  SPI_ReceiveData()
  
/** Macro specifyng the radio SPI busy flag.
 *
 */
#define HAL_NRF_HW_SPI_BUSY   (RESET == SPI_GetFlagStatus(SPI_FLAG_RXNE))

/**
 * Pulses the CE to nRF24L01 for at least 10 us
 */

#define CE_PULSE() do { \
  uint8_t count; \
  count = 20; \
  CE_HIGH();  \
  while(count--){} \
  CE_LOW();  \
  } while(false)

 
#endif // HAL_NRF_STM8L101XX_H__

/** @} */