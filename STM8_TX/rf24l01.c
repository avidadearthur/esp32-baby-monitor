#include "rf24l01.h"
#include <stm8s_spi.h>
#include <stm8s_gpio.h>

uint8_t * val_test = 0;

RF24L01_reg_SETUP_AW_content SETUP_AW;
RF24L01_reg_EN_AA_content EN_AA;
RF24L01_reg_EN_RXADDR_content RX_ADDR;
RF24L01_reg_RF_CH_content RF_CH;
RF24L01_reg_RX_PW_P0_content RX_PW_P0;
RF24L01_reg_RF_SETUP_content RF_SETUP;
RF24L01_reg_CONFIG_content config;

uint8_t status, i;

RF24L01_reg_STATUS_content a;

uint16_t delay;

uint8_t res;


void RF24L01_init(void) {
  //Chip Select(CSN)
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
      SPI_BAUDRATEPRESCALER_4,
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
  
  //Chip unselect
  GPIO_WriteHigh(GPIOC, GPIO_PIN_4);
}

/*** This function writes the address of the  
* TX pipe in the TX_ADDR register.
*/
void RF24L01_write_address(uint8_t *tx_addr) {
  uint8_t i;
  // Pull the CS Pin LOW to select the device
	GPIO_WriteLow(GPIOC, GPIO_PIN_4);
  
  //Send address and write command
  while (SPI_GetFlagStatus(SPI_FLAG_TXE)== RESET);
  SPI_SendData(RF24L01_command_W_REGISTER | RF24L01_reg_TX_ADDR);
  while (SPI_GetFlagStatus(SPI_FLAG_BSY)== SET);
  while (SPI_GetFlagStatus(SPI_FLAG_RXNE)== RESET);
  SPI_ReceiveData();

  //Send data  
  for (i=0; i<5; i++) {
    while (SPI_GetFlagStatus(SPI_FLAG_TXE)== RESET);
    SPI_SendData(tx_addr[i]);
    while (SPI_GetFlagStatus(SPI_FLAG_BSY)== SET);
    while (SPI_GetFlagStatus(SPI_FLAG_RXNE)== RESET);
    SPI_ReceiveData();
  }
  
  // Pull the CS HIGH to release the device
	GPIO_WriteHigh(GPIOC, GPIO_PIN_4);
}


RF24L01_reg_STATUS_content RF24L01_get_status(void) {
  
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
  //uint16_t delay = 0xFF;
	delay = 0xFF;
  while(delay--);
  GPIO_WriteLow(GPIOC, GPIO_PIN_3);
}


uint8_t RF24L01_is_data_available(void) {
  //RF24L01_reg_STATUS_content a;
  a = RF24L01_get_status();
  return a.RX_DR;
}

void RF24L01_clear_interrupts(void) {
  //RF24L01_reg_STATUS_content a;
  a = RF24L01_get_status();
  RF24L01_write_register(RF24L01_reg_STATUS, (uint8_t*)&a, 1);
}

void setup2(uint8_t *raddr, uint8_t *taddr){
	
	GPIO_WriteLow(GPIOC, GPIO_PIN_3); //CE -> Low
	*(val_test) = 0x0B; // Has to be set by RX or TX
	RF24L01_write_register(0x00, val_test, 1); //CONFIG
	*(val_test) = 0x3F; // 00111111
	RF24L01_write_register(0x01, val_test, 1); //EN_AA 
	*(val_test) = 0x03; // 00000011
	RF24L01_write_register(0x02, val_test, 1); // EN_RXADDR
	*(val_test) = 0x03; // 00000011
	RF24L01_write_register(0x03, val_test, 1); // SETUP_AW
	*(val_test) = 0x0F; // 00001111 (0x5F)
	RF24L01_write_register(0x04, val_test, 1); // SETUP_RETR (Automatic Retransmission)
	*(val_test) = 0x1C; // 00011100 
	RF24L01_write_register(0x05, val_test, 1); // RF_CH (RF channel = 28)
	*(val_test) = 0x05; // 00000010	(0x05)
	RF24L01_write_register(0x06, val_test, 1); // RF_SETUP (1Mbps, -12dB)
	*(val_test) = 0x0E; // 00001110
	RF24L01_write_register(0x07, val_test, 1); // STATUS
	*(val_test) = 0x0F; // 00001111
	RF24L01_write_register(0x08, val_test, 1); // OBSERVE_TX
	*(val_test) = 0x00; // 00000000
	RF24L01_write_register(0x09, val_test, 1); // RPD (Received Power Detector)
	*(val_test) = 0x41; // 01000001
	RF24L01_write_register(0x0a, val_test, 1); // RX_ADDR_P0
	*(val_test) = 0x41;
	RF24L01_write_register(0x0b, val_test, 1); // RX_ADDR_P1
	*(val_test) = 0xC3;
	RF24L01_write_register(0x0c, val_test, 1); // RX_ADDR_P2
	*(val_test) = 0xC4;
	RF24L01_write_register(0x0d, val_test, 1); // RX_ADDR_P3
	*(val_test) = 0xC5;
	RF24L01_write_register(0x0e, val_test, 1); // RX_ADDR_P4	
	*(val_test) = 0xC6;
	RF24L01_write_register(0x0f, val_test, 1); // RX_ADDR_P5
	*(val_test) = 0x41;
	RF24L01_write_register(0x10, val_test, 1); // TX_ADDR
	*(val_test) = 0x04;
	RF24L01_write_register(0x11, val_test, 1); // RX_PW_P0
	*(val_test) = 0x04;
	RF24L01_write_register(0x12, val_test, 1); // RX_PW_P1
	*(val_test) = 0x04;
	RF24L01_write_register(0x13, val_test, 1); // RX_PW_P2
	*(val_test) = 0x04;
	RF24L01_write_register(0x14, val_test, 1); // RX_PW_P3
	*(val_test) = 0x04;
	RF24L01_write_register(0x15, val_test, 1); // RX_PW_P4
	*(val_test) = 0x04;
	RF24L01_write_register(0x16, val_test, 1); // RX_PW_P5
	*(val_test) = 0x11; // 0001 0001
	RF24L01_write_register(0x17, val_test, 1); // FIFO_STATUS
	*(val_test) = 0x00;
	RF24L01_write_register(0x18, val_test, 1); 
	*(val_test) = 0x00;
	RF24L01_write_register(0x19, val_test, 1); 
	*(val_test) = 0x00;
	RF24L01_write_register(0x1a, val_test, 1); 
	*(val_test) = 0x00;
	RF24L01_write_register(0x1b, val_test, 1);
	*(val_test) = 0x00;
	RF24L01_write_register(0x1c, val_test, 1); // DYNPD
	*(val_test) = 0x00;
	RF24L01_write_register(0x1d, val_test, 1); // FEATURE


// set receive address 0 - equal to transmission to allow auto ack
  RF24L01_write_register(RF24L01_reg_RX_ADDR_P0, raddr, 5);
	RF24L01_write_register(RF24L01_reg_TX_ADDR, taddr, 5);
}

void NRF_MODE_RX(void){

  //Clear the status register to discard any data in the buffers - THIS I GOT FROM THEM!!!!!!!!!!
  
  *((uint8_t *) &a) = 0;
  a.RX_DR = 1;
  a.MAX_RT = 1;
  a.TX_DS = 1;
  RF24L01_write_register(RF24L01_reg_STATUS, (uint8_t *) &a, 1);
  RF24L01_send_command(RF24L01_command_FLUSH_RX);
  
//set config for RX
*(val_test) = 0x0B; // 00001011
RF24L01_write_register(0x00, val_test, 1);

//set RPD for RX
*(val_test) = 0x00;
RF24L01_write_register(0x09, val_test, 1);	
	
  GPIO_WriteHigh(GPIOC, GPIO_PIN_3); //CE -> High
}


void NRF_MODE_TX(void){
  RF24L01_send_command(RF24L01_command_FLUSH_TX);
  GPIO_WriteLow(GPIOC, GPIO_PIN_3);//select CE

	//set config for TX
	*(val_test) = 0x0A; // 00001010
	RF24L01_write_register(0x00, val_test, 1);
	
	//set RPD for TX
	*(val_test) = 0x01;
	RF24L01_write_register(0x09, val_test, 1);
}
