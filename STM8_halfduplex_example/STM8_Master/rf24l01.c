#include "rf24l01.h"
#include <stm8s_spi.h>
#include <stm8s_gpio.h>

void RF24L01_init(void) {
  //Chip Select
  GPIO_Init(
    GPIOC,
    GPIO_PIN_4,
    GPIO_MODE_OUT_PP_HIGH_FAST
  );
  GPIO_WriteHigh(GPIOC, GPIO_PIN_4);

  //CE
  GPIO_Init(
    GPIOC,
    GPIO_PIN_3,
    GPIO_MODE_OUT_PP_HIGH_FAST
  );
  GPIO_WriteLow(GPIOC, GPIO_PIN_3);

  //SPI
  SPI_Init(
      SPI_FIRSTBIT_MSB,
      SPI_BAUDRATEPRESCALER_256,
      SPI_MODE_MASTER,
      SPI_CLOCKPOLARITY_LOW,
      SPI_CLOCKPHASE_1EDGE,
      SPI_DATADIRECTION_2LINES_FULLDUPLEX,
      SPI_NSS_SOFT,
      (uint8_t)0x07
  );
  SPI_Cmd(ENABLE);
  
}

void RF24L01_send_command(uint8_t command) {
  //Chip select
  GPIO_WriteLow(GPIOC, GPIO_PIN_4);
  
  //Send command
  while (SPI_GetFlagStatus(SPI_FLAG_TXE)== RESET);
  SPI_SendData(command);
  while (SPI_GetFlagStatus(SPI_FLAG_BSY)== SET);
  while (SPI_GetFlagStatus(SPI_FLAG_RXNE)== RESET);
  SPI_ReceiveData();
  
  //Chip select
  GPIO_WriteHigh(GPIOC, GPIO_PIN_4);
}

uint8_t RF24L01_read_register(uint8_t register_addr) {
  uint8_t result;
  //Chip select
  GPIO_WriteLow(GPIOC, GPIO_PIN_4);
  
  //Send address and read command
  while (SPI_GetFlagStatus(SPI_FLAG_TXE)== RESET);
  SPI_SendData(RF24L01_command_R_REGISTER | register_addr);
  while (SPI_GetFlagStatus(SPI_FLAG_BSY)== SET);
  while (SPI_GetFlagStatus(SPI_FLAG_RXNE)== RESET);
  SPI_ReceiveData();
  
  //Get data
  while (SPI_GetFlagStatus(SPI_FLAG_TXE)== RESET);
  SPI_SendData(0x00);
  while (SPI_GetFlagStatus(SPI_FLAG_BSY)== SET);
  while (SPI_GetFlagStatus(SPI_FLAG_RXNE) == RESET);
  result = SPI_ReceiveData();
  
  //Chip select
  GPIO_WriteHigh(GPIOC, GPIO_PIN_4);
  
  return result;
}

void RF24L01_write_register(uint8_t register_addr, uint8_t *value, uint8_t length) {
  uint8_t i;
  //Chip select
  GPIO_WriteLow(GPIOC, GPIO_PIN_4);
  
  //Send address and write command
  while (SPI_GetFlagStatus(SPI_FLAG_TXE)== RESET);
  SPI_SendData(RF24L01_command_W_REGISTER | register_addr);
  while (SPI_GetFlagStatus(SPI_FLAG_BSY)== SET);
  while (SPI_GetFlagStatus(SPI_FLAG_RXNE)== RESET);
  SPI_ReceiveData();

  //Send data  
  for (i=0; i<length; i++) {
    while (SPI_GetFlagStatus(SPI_FLAG_TXE)== RESET);
    SPI_SendData(value[i]);
    while (SPI_GetFlagStatus(SPI_FLAG_BSY)== SET);
    while (SPI_GetFlagStatus(SPI_FLAG_RXNE)== RESET);
    SPI_ReceiveData();
  }
  
  //Chip select
  GPIO_WriteHigh(GPIOC, GPIO_PIN_4);
}

void RF24L01_setup(uint8_t *tx_addr, uint8_t *rx_addr, uint8_t channel) {
  GPIO_WriteLow(GPIOC, GPIO_PIN_3); //CE -> Low

  RF24L01_reg_SETUP_AW_content SETUP_AW;
  *((uint8_t *)&SETUP_AW) = 0;
  SETUP_AW.AW = 0x03;
  RF24L01_write_register(RF24L01_reg_SETUP_AW, ((uint8_t *)&SETUP_AW), 1);
  
  RF24L01_write_register(RF24L01_reg_RX_ADDR_P0, rx_addr, 5);
  RF24L01_write_register(RF24L01_reg_TX_ADDR, tx_addr, 5);

  RF24L01_reg_EN_AA_content EN_AA;
  *((uint8_t *)&EN_AA) = 0;
  RF24L01_write_register(RF24L01_reg_EN_AA, ((uint8_t *)&EN_AA), 1);
  
  RF24L01_reg_EN_RXADDR_content RX_ADDR;
  *((uint8_t *)&RX_ADDR) = 0;
  RX_ADDR.ERX_P0 = 1;
  RF24L01_write_register(RF24L01_reg_EN_RXADDR, ((uint8_t *)&RX_ADDR), 1);

  RF24L01_reg_RF_CH_content RF_CH;
  *((uint8_t *)&RF_CH) = 0;
  RF_CH.RF_CH = channel;
  RF24L01_write_register(RF24L01_reg_RF_CH, ((uint8_t *)&RF_CH), 1);

  RF24L01_reg_RX_PW_P0_content RX_PW_P0;
  *((uint8_t *)&RX_PW_P0) = 0;
  RX_PW_P0.RX_PW_P0 = 0x20;
  RF24L01_write_register(RF24L01_reg_RX_PW_P0, ((uint8_t *)&RX_PW_P0), 1);  

  RF24L01_reg_RF_SETUP_content RF_SETUP;
  *((uint8_t *)&RF_SETUP) = 0;
  RF_SETUP.RF_PWR = 0x03;
  RF_SETUP.RF_DR = 0x01;
  RF_SETUP.LNA_HCURR = 0x01;
  RF24L01_write_register(RF24L01_reg_RF_SETUP, ((uint8_t *)&RF_SETUP), 1);
  
  RF24L01_reg_CONFIG_content config;
  *((uint8_t *)&config) = 0;
  config.PWR_UP = 0;
  config.PRIM_RX = 1;
  config.EN_CRC = 1;
  config.MASK_MAX_RT = 0;
  config.MASK_TX_DS = 0;
  config.MASK_RX_DR = 0;
  RF24L01_write_register(RF24L01_reg_CONFIG, ((uint8_t *)&config), 1);
}

void RF24L01_set_mode_TX(void) {
  RF24L01_send_command(RF24L01_command_FLUSH_TX);
  GPIO_WriteLow(GPIOC, GPIO_PIN_3);

  RF24L01_reg_CONFIG_content config;
  *((uint8_t *)&config) = 0;
  config.PWR_UP = 1;
  config.EN_CRC = 1;
  config.MASK_MAX_RT = 0;
  config.MASK_TX_DS = 0;
  config.MASK_RX_DR = 0;
  RF24L01_write_register(RF24L01_reg_CONFIG, ((uint8_t *)&config), 1);  
}

void RF24L01_set_mode_RX(void) {
  RF24L01_reg_CONFIG_content config;
  *((uint8_t *)&config) = 0;
  config.PWR_UP = 1;
  config.PRIM_RX = 1;
  config.EN_CRC = 1;
  config.MASK_MAX_RT = 0;
  config.MASK_TX_DS = 0;
  config.MASK_RX_DR = 0;
  RF24L01_write_register(RF24L01_reg_CONFIG, ((uint8_t *)&config), 1);

  //Clear the status register to discard any data in the buffers
  RF24L01_reg_STATUS_content a;
  *((uint8_t *) &a) = 0;
  a.RX_DR = 1;
  a.MAX_RT = 1;
  a.TX_DS = 1;
  RF24L01_write_register(RF24L01_reg_STATUS, (uint8_t *) &a, 1);
  RF24L01_send_command(RF24L01_command_FLUSH_RX);
  
  GPIO_WriteHigh(GPIOC, GPIO_PIN_3); //CE -> High
}

RF24L01_reg_STATUS_content RF24L01_get_status(void) {
  uint8_t status;
  //Chip select
  GPIO_WriteLow(GPIOC, GPIO_PIN_4);
  
  //Send address and command
  while (SPI_GetFlagStatus(SPI_FLAG_TXE)== RESET);
  SPI_SendData(RF24L01_command_NOP);
  while (SPI_GetFlagStatus(SPI_FLAG_BSY)== SET);
  while (SPI_GetFlagStatus(SPI_FLAG_RXNE)== RESET);
  status = SPI_ReceiveData();
  
  //Chip select
  GPIO_WriteHigh(GPIOC, GPIO_PIN_4);

  return *((RF24L01_reg_STATUS_content *) &status);
}

void RF24L01_write_payload(uint8_t *data, uint8_t length) {
  RF24L01_reg_STATUS_content a;
  a = RF24L01_get_status();
  if (a.MAX_RT == 1) {
    //If MAX_RT, clears it so we can send data
    *((uint8_t *) &a) = 0;
    a.TX_DS = 1;
    RF24L01_write_register(RF24L01_reg_STATUS, (uint8_t *) &a, 1);
  }
  
  uint8_t i;
  //Chip select
  GPIO_WriteLow(GPIOC, GPIO_PIN_4);
  
  //Send address and command
  while (SPI_GetFlagStatus(SPI_FLAG_TXE)== RESET);
  SPI_SendData(RF24L01_command_W_TX_PAYLOAD);
  while (SPI_GetFlagStatus(SPI_FLAG_BSY)== SET);
  while (SPI_GetFlagStatus(SPI_FLAG_RXNE)== RESET);
  SPI_ReceiveData();

  //Send data
  for (i=0; i<length; i++) {
    while (SPI_GetFlagStatus(SPI_FLAG_TXE)== RESET);
    SPI_SendData(data[i]);
    while (SPI_GetFlagStatus(SPI_FLAG_BSY)== SET);
    while (SPI_GetFlagStatus(SPI_FLAG_RXNE)== RESET);
    SPI_ReceiveData();
  }
  
  //Chip select
  GPIO_WriteHigh(GPIOC, GPIO_PIN_4);
  
  //Generates an impulsion for CE to send the data
  GPIO_WriteHigh(GPIOC, GPIO_PIN_3);
  uint16_t delay = 0xFF;
  while(delay--);
  GPIO_WriteLow(GPIOC, GPIO_PIN_3);
}


void RF24L01_read_payload(uint8_t *data, uint8_t length) {
  uint8_t i, status;
  //Chip select
  GPIO_WriteLow(GPIOC, GPIO_PIN_4);
  
  //Send address
  while (SPI_GetFlagStatus(SPI_FLAG_TXE)== RESET);
  SPI_SendData(RF24L01_command_R_RX_PAYLOAD);
  while (SPI_GetFlagStatus(SPI_FLAG_BSY)== SET);
  while (SPI_GetFlagStatus(SPI_FLAG_RXNE)== RESET);
  status = SPI_ReceiveData();

  //Get data
  for (i=0; i<length; i++) {
    while (SPI_GetFlagStatus(SPI_FLAG_TXE)== RESET);
    SPI_SendData(0x00);
    while (SPI_GetFlagStatus(SPI_FLAG_BSY)== SET);
    *(data++) = SPI_ReceiveData();
  }
  
  //Chip select
  GPIO_WriteHigh(GPIOC, GPIO_PIN_4); 
  
  RF24L01_write_register(RF24L01_reg_STATUS, &status, 1);
  RF24L01_send_command(RF24L01_command_FLUSH_RX);
}

uint8_t RF24L01_was_data_sent(void) {
  RF24L01_reg_STATUS_content a;
  a = RF24L01_get_status();
  
  uint8_t res = 0;
  if (a.TX_DS) {
    res = 1;
  }
  else if (a.MAX_RT) {
    res = 2;
  }
  
  return res;
}

uint8_t RF24L01_is_data_available(void) {
  RF24L01_reg_STATUS_content a;
  a = RF24L01_get_status();
  return a.RX_DR;
}

void RF24L01_clear_interrupts(void) {
  RF24L01_reg_STATUS_content a;
  a = RF24L01_get_status();
  RF24L01_write_register(RF24L01_reg_STATUS, (uint8_t*)&a, 1);
}
