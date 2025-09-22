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
       pdm_microphone();
  
       void setLEDbrightness (const int32_t bright10 ) {};
       void setDriveLED(int on) {};
       void setGPIO(const int g) {};

       int start();

       int16_t *read( int16_t* buffer, const int32_t samples);
    
       int32_t buffered();
       void    dma_handler();
};
#endif
