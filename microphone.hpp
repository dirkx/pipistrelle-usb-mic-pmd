/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * General microphone class; used by both the PDM and Analogue microphone.
 *.
 */

#ifndef _PICO_MICROPHONE_HPP_
#define _PICO_MICROPHONE_HPP_

#include "filter.hpp"

class microphone {
    public :
    
       void setLEDbrightness (const int32_t bright10 ) {};
       void setDriveLED(int on) {};
       void setGPIO(const int g) {};

       virtual int start();

       virtual int16_t *read( int16_t* buffer, const int32_t samples);
    
       virtual int32_t buffered();
       virtual void    dma_handler();
};
#endif
