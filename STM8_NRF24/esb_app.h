
#ifndef _ESB_APP_H_
#define _ESB_APP_H_

#include <stdint.h>

#include "hal_nrf.h"
#include "hal_nrf_hw.h"


#define ESB_MAX_PAYLOAD_LEN											(32)
#define ESB_MAX_ACK_PAYLOAD_LEN									(4) //DOC said it is 32, but my tring revealed it being 4 


#define TX_PIPE													(HAL_NRF_PIPE3)
#define RX_PIPE													(HAL_NRF_PIPE3)


void esb_init(void);

/*for PTX*/
void esb_send_data(hal_nrf_address_t tx_pipe_number, 
	uint8_t *p_data, uint8_t len);
	
uint8_t  is_esb_ack_reached(void);

void esb_fetch_ack_data(uint8_t *p_data, uint8_t *p_len);

uint8_t is_esb_max_retry_count_reached(void);

void esb_max_retry_count_reached_has_been_handled(void);


/*for PRX*/
void esb_fetch_received_data(hal_nrf_address_t *p_pipe, 
	uint8_t *p_data, uint8_t *p_len);
	
uint8_t is_esb_received_data(void);

void esb_send_ack_data(uint8_t *p_data, uint8_t len);

void esb_receiving_event_has_been_handled(void);

/*nRF24L01 IRQ*/
#if !defined(NRF24LE1_H_) && !defined(NRF24LU1P_H)
void esb_irq(void);
#endif

#endif