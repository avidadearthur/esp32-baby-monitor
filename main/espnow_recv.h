#ifndef ESPNOW_RECV_H
#define ESPNOW_RECV_H

#include "config.h"

void espnow_recv_task(const uint8_t* mac_addr, const uint8_t* data, int len);
void init_recv(StreamBufferHandle_t network_stream_buf);

#endif
