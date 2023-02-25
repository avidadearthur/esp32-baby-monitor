#include "rf24l01.h"

/*
RF24L01 stm8s demo
MASTER
This code sends two 32 bit ints to a slave, and gets the result of the basic
mathematic operations involving them.

This is a proof of concept and not a copy and use library.
Adapt this code to your application.

Erwan Martin
Fall 2013
ARENIB Delta
http://delta.arenib.org/

License: MIT

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

uint8_t mutex;

typedef struct _data_to_send {
  uint32_t op1;
  uint32_t op2;
} data_to_send;
data_to_send to_send;

typedef struct _data_received {
  uint32_t add;
  uint32_t sub;
  uint32_t mult;
  uint32_t div;
} data_received;
data_received received;

int main( void )
{
  RF24L01_init();
  uint8_t tx_addr[5] = {0x04, 0xAD, 0x45, 0x98, 0x51};
  uint8_t rx_addr[5] = {0x44, 0x88, 0xBA, 0xBE, 0x42};
  RF24L01_setup(tx_addr, rx_addr, 12);
  RF24L01_set_mode_TX();
  
  //IRQ
  GPIO_Init(
    GPIOC,
    GPIO_PIN_2,
    GPIO_MODE_IN_FL_IT
  );
  EXTI_SetExtIntSensitivity(EXTI_PORT_GPIOC, EXTI_SENSITIVITY_FALL_ONLY);
  enableInterrupts();
  
  to_send.op1 = 1;
  to_send.op2 = 1;
  
  while(1) {
      //Prepare the buffer to send from the data_to_send struct
      uint8_t buffer_to_send[32];
      uint8_t i = 0;
      for (i=0; i<32; i++) {
        buffer_to_send[i] = 0;
      }
      *((data_to_send *) &buffer_to_send) = to_send;

      //Send the buffer
      mutex = 0;
      RF24L01_set_mode_TX();
      RF24L01_write_payload(buffer_to_send, 32);

      //Wait for the buffer to be sent
      while(!mutex);
      if (mutex != 1) {
        //The transmission failed
      }
      
      //Wait for the response
      //TODO: implement a timeout if nothing is received after a certain amount of time
      mutex = 0;
      RF24L01_set_mode_RX();
      while(!mutex);
      if (mutex == 1) {
        uint8_t recv_data[32];
        RF24L01_read_payload(recv_data, 32);
        received = *((data_received *) &recv_data);
        
        asm("nop"); //Place a breakpoint here to see memory
      }
      else {
        //Something happened
      }
      
      //Let's vary the data to send
      to_send.op1++;
      to_send.op2 += 2;
  }
}

INTERRUPT_HANDLER(EXTI_PORTC_IRQHandler, 5) {
  uint8_t sent_info;
  if (sent_info = RF24L01_was_data_sent()) {
    //Packet was sent or max retries reached
    mutex = sent_info;
    RF24L01_clear_interrupts();
    return;
  }

  if(RF24L01_is_data_available()) {
    //Packet was received
    mutex = 1;
    RF24L01_clear_interrupts();
    return;
  }
  
  RF24L01_clear_interrupts();
}


