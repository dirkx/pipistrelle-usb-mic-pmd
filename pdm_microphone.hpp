/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * General microphone class; used by both the PDM and Analogue microphone.
 *.
 */

#ifndef _PICO_PDM_MICROPHONE_HPP_
#define _PICO_PDM_MICROPHONE_HPP_

#include "microphone.hpp"
#include "filter.hpp"
#define PDM (0x0050444d)

class pdm_microphone : microphone {
    public :
       typedef void (*C_DMA_handler)(uint8_t * buffer, size_t len);

       pdm_microphone(
		const int32_t sampleRate, 
		const int32_t gpio_dat, 
		const int32_t gpio_clk, 
		C_DMA_handler pDMAhandler );
  
       void setLEDbrightness (const int32_t bright10 ) {};
       void setDriveLED(int on) {};
       void setGPIO(const int g) {};

       int start();

       int16_t *read(int16_t* buffer, const int32_t samples);
    
       int32_t buffered();
       void dma_handler(uint8_t * buff, size_t len);
    private:
       C_DMA_handler DMAhandler;
       uint _gpio_dat, _gpio_clk;
       size_t _buff_len, _buff_idx, _buff_size;
       uint8_t * _buff;
       int32_t _sampleRate;
};
#endif
