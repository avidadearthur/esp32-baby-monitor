#ifndef RF24L01_H
#define RF24L01_H

#include <stm8s.h> //Required for the stdint typedefs

#define RF24L01_reg_CONFIG      0x00
#define RF24L01_reg_EN_AA       0x01
#define RF24L01_reg_EN_RXADDR   0x02
#define RF24L01_reg_SETUP_AW    0x03
#define RF24L01_reg_SETUP_RETR  0x04
#define RF24L01_reg_RF_CH       0x05
#define RF24L01_reg_RF_SETUP    0x06
#define RF24L01_reg_STATUS      0x07
#define RF24L01_reg_OBSERVE_TX  0x08
#define RF24L01_reg_CD          0x09
#define RF24L01_reg_RX_ADDR_P0  0x0A
#define RF24L01_reg_RX_ADDR_P1  0x0B
#define RF24L01_reg_RX_ADDR_P2  0x0C
#define RF24L01_reg_RX_ADDR_P3  0x0D
#define RF24L01_reg_RX_ADDR_P4  0x0E
#define RF24L01_reg_RX_ADDR_P5  0x0F
#define RF24L01_reg_TX_ADDR     0x10
#define RF24L01_reg_RX_PW_P0    0x11
#define RF24L01_reg_RX_PW_P1    0x12
#define RF24L01_reg_RX_PW_P2    0x13
#define RF24L01_reg_RX_PW_P3    0x14
#define RF24L01_reg_RX_PW_P4    0x15
#define RF24L01_reg_RX_PW_P5    0x16
#define RF24L01_reg_FIFO_STATUS 0x17
#define RF24L01_reg_DYNPD       0x1C
#define RF24L01_reg_FEATURE     0x1D

#define RF24L01_command_R_REGISTER            0x00
#define RF24L01_command_W_REGISTER            0x20
#define RF24L01_command_R_RX_PAYLOAD          0x61
#define RF24L01_command_W_TX_PAYLOAD          0xA0
#define RF24L01_command_FLUSH_TX              0xE1
#define RF24L01_command_FLUSH_RX              0xE2
#define RF24L01_command_REUSE_TX_PL           0xE3
#define RF24L01_command_ACTIVATE              0x50
#define RF24L01_command_R_RX_PL_WID           0x60
#define RF24L01_command_W_ACK_PAYLOAD         0xA8
#define RF24L01_command_W_TX_PAYLOAD_NOACK    0x58
#define RF24L01_command_NOP                   0xFF

//Make sure your compiler orders the bits correctly in the structs
#pragma bitfields=reversed

typedef struct _RF24L01_reg_CONFIG_content {
  uint8_t reserved     :1;
  uint8_t MASK_RX_DR   :1;
  uint8_t MASK_TX_DS   :1;
  uint8_t MASK_MAX_RT  :1;
  uint8_t EN_CRC       :1;
  uint8_t CRCO         :1;
  uint8_t PWR_UP       :1;
  uint8_t PRIM_RX      :1;
} RF24L01_reg_CONFIG_content;

typedef struct _RF24L01_reg_EN_AA_content {
  uint8_t reserved :2;
  uint8_t ENAA_P5  :1;
  uint8_t ENAA_P4  :1;
  uint8_t ENAA_P3  :1;
  uint8_t ENAA_P2  :1;
  uint8_t ENAA_P1  :1;
  uint8_t ENAA_P0  :1;
  
} RF24L01_reg_EN_AA_content;

typedef struct _RF24L01_reg_EN_RXADDR_content {
  uint8_t reserved :2;
  uint8_t ERX_P5  :1;
  uint8_t ERX_P4  :1;
  uint8_t ERX_P3  :1;
  uint8_t ERX_P2  :1;
  uint8_t ERX_P1  :1;
  uint8_t ERX_P0  :1;
  
} RF24L01_reg_EN_RXADDR_content;

typedef struct _RF24L01_reg_SETUP_AW_content {
  uint8_t reserved :6;
  uint8_t AW       :2;
} RF24L01_reg_SETUP_AW_content;

typedef struct _RF24L01_reg_SETUP_RETR_content {
  uint8_t ARD :4;
  uint8_t ARC :4;
} RF24L01_reg_SETUP_RETR_content;

typedef struct _RF24L01_reg_RF_CH_content {
  uint8_t reserved :1;
  uint8_t RF_CH    :7;
} RF24L01_reg_RF_CH_content;

typedef struct _RF24L01_reg_RF_SETUP_content {
  uint8_t reserved  :3;
  uint8_t PLL_LOCK  :1;
  uint8_t RF_DR     :1;
  uint8_t RF_PWR    :2;
  uint8_t LNA_HCURR :1;
} RF24L01_reg_RF_SETUP_content;

typedef struct _RF24L01_reg_STATUS_content {
  uint8_t reserved  :1;
  uint8_t RX_DR     :1;
  uint8_t TX_DS     :1;
  uint8_t MAX_RT    :1;
  uint8_t RX_P_NO   :3;
  uint8_t TX_FULL   :1;
} RF24L01_reg_STATUS_content;

typedef struct _RF24L01_reg_OBSERVE_TX_content {
  uint8_t PLOS_CNT  :4;
  uint8_t ARC_CNT   :4;
} RF24L01_reg_OBSERVE_TX_content;

typedef struct _RF24L01_reg_CD_content {
  uint8_t reserved  :7;
  uint8_t CD        :1;
} RF24L01_reg_CD_content;

typedef struct _RF24L01_reg_RX_PW_P0_content {
  uint8_t reserved  :2;
  uint8_t RX_PW_P0  :6;
} RF24L01_reg_RX_PW_P0_content;

typedef struct _RF24L01_reg_RX_PW_P1_content {
  uint8_t reserved  :2;
  uint8_t RX_PW_P1  :6;
} RF24L01_reg_RX_PW_P1_content;

typedef struct _RF24L01_reg_RX_PW_P2_content {
  uint8_t reserved  :2;
  uint8_t RX_PW_P2  :6;
} RF24L01_reg_RX_PW_P2_content;

typedef struct _RF24L01_reg_RX_PW_P3_content {
  uint8_t reserved  :2;
  uint8_t RX_PW_P3  :6;
} RF24L01_reg_RX_PW_P3_content;

typedef struct _RF24L01_reg_RX_PW_P4_content {
  uint8_t reserved  :2;
  uint8_t RX_PW_P4  :6;
} RF24L01_reg_RX_PW_P4_content;

typedef struct _RF24L01_reg_RX_PW_P5_content {
  uint8_t reserved  :2;
  uint8_t RX_PW_P5  :6;
} RF24L01_reg_RX_PW_P5_content;

typedef struct _RF24L01_reg_FIFO_STATUS_content {
  uint8_t reserved  :1;
  uint8_t TX_REUSE  :1;
  uint8_t TX_FULL   :1;
  uint8_t TX_EMPTY  :1;
  uint8_t reserved2 :2;
  uint8_t RX_FULL   :1;
  uint8_t RX_EMPTY  :1;
} RF24L01_reg_FIFO_STATUS_content;

typedef struct _RF24L01_reg_DYNPD_content {
  uint8_t reserved  :2;
  uint8_t DPL_P5    :1;
  uint8_t DPL_P4    :1;
  uint8_t DPL_P3    :1;
  uint8_t DPL_P2    :1;
  uint8_t DPL_P1    :1;
  uint8_t DPL_P0    :1;
} RF24L01_reg_DYNPD_content;

typedef struct _RF24L01_reg_FEATURE_content {
  uint8_t reserved   :4;
  uint8_t EN_DPL     :2;
  uint8_t EN_ACK_PAY :1;
  uint8_t EN_DYN_ACK :1;
} RF24L01_reg_FEATURE_content;

void RF24L01_init(void);
void RF24L01_setup(uint8_t *tx_addr, uint8_t *rx_addr, uint8_t channel);
void RF24L01_set_mode_TX(void);
void RF24L01_set_mode_RX(void);
uint8_t RF24L01_was_data_sent(void);
uint8_t RF24L01_is_data_available(void);
void RF24L01_read_payload(uint8_t *data, uint8_t length);
void RF24L01_write_payload(uint8_t *data, uint8_t length);
void RF24L01_clear_interrupts(void);

#endif