#ifndef FFTPEAK_H
#define FFTPEAK_H

#include <stdio.h>
#include <fastmath.h>
#include <complex.h>
#include "config.h"
#include "fft.h"
#include "driver/spi_master.h"
#include "soc/gpio_struct.h"
#include "driver/uart.h"
#include "soc/uart_struct.h"
#include <math.h>

#include "esp_dsp.h"


void init_fft(StreamBufferHandle_t fft_stream_buf);

#endif