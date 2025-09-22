/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * General microphone class; used by both the PDM and Analogue microphone.
 *.
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

#include "pdm_receiver.h"

#include "filter.hpp"
#include "microphone.hpp"
#include "pdm_microphone.hpp"

#define Q (8)

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

uint8_t * _buffer;

pdm_microphone::pdm_microphone(
                uint sampleRate,
                uint gpio_dat,
                uint gpio_clk, 
                C_DMA_handler pDMAhandler )
{
  _gpio_dat = gpio_dat;
  _gpio_clk = gpio_clk;
  _sampleRate = sampleRate;

  size_t nSamples = sampleRate / 1000;  // 1000 USB blocks per second.

  _buff_size = 4 * nSamples;
  _buffer = (uint8_t*)malloc(_buff_size);
  hard_assert(_buffer);

  _buff_len = 0;
  _buff_idx = 0;

  printf("INit PDM: ");
  init_pdm_receiver(pDMAhandler, _gpio_dat, _gpio_clk, nSamples, Q);
  printf("Ok\n");

  return;
}
  
int pdm_microphone::start() { 
  receiver_clock_start(_gpio_clk,_sampleRate);
  return 0; 
};

// Read from our buffer until it is empty; then reset it - so
// we're not doig round robin - but simply wait until it is
// empty.
//
int16_t *pdm_microphone::read( int16_t* buffer, const int32_t samples) { 
  int32_t n = (int16_t) min(samples, _buff_len - _buff_idx);

  if (n) {
     for(size_t i = 0; i < n; i++, _buff_idx++) {
	// expand the buffer into a format that
	// is suitable to send as USB packets. This
	// is a bit double - we could do this directly
	// in the callback handler in pdm_receive to
	// skip a loop.
	//
	buffer[i] = _buffer[_buff_idx] * 200; // from uint8 to a int16
     };

     if (_buff_idx == _buff_len)
	_buff_idx = _buff_len = 0;
  };

  return buffer;
};
    
int32_t pdm_microphone::buffered() { 
  return _buff_len - _buff_idx;
};

// Append to our buffer until we are full.
//
void pdm_microphone::dma_handler(uint8_t * buff, size_t len) {
  len = min(len, _buff_size - _buff_len);
  memcpy(_buffer + _buff_len, buff, len);
  _buff_len += len;
  return;
};
