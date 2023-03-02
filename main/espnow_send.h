#ifndef ESPNOW_SEND_H
#define ESPNOW_SEND_H

#include "config.h"

void espnow_send_task(void* task_param);
void init_transmit(StreamBufferHandle_t mic_stream_buf);

#endif
