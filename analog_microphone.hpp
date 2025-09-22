/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Tons of additional stuff (c) Phil Atkin 2022-2024
 */

#ifndef _PICO_ANALOG_MICROPHONE_HPP_
#define _PICO_ANALOG_MICROPHONE_HPP_

#define ANALOG (0x616e616c)

#include "filter.hpp"
#include "microphone.hpp"

class analog_microphone : microphone {
    public : 
    typedef void (*C_DMA_handler)();
    analog_microphone() {};

    private :
    
    // for the startup pulses
    int32_t persistentBrightness = 0;
    
    int32_t PWMLEDvalue = 0;
    int32_t PWMLEDerror = 0;

    short nextWobulatedSample ();

    void LEDupdate(const short *p, const int32_t ix0, const int32_t items );

    void wobulate ( const int16_t *fromHere, const int32_t items );

    int32_t buffersWritten = 0;
        
    // Pico hardware stuff
    uint    gpio;
    int     dma_channel;
    uint    dma_irq;
    
    // UGLY - SDK needs an external function with C binding to call the method
    C_DMA_handler DMAhandler;
    
    // buffering and decoupling
    volatile uint64_t analog_wix; // these will not overflow for 684k years
    volatile uint64_t analog_rix;
    
    int32_t phase;
    int32_t readops;
    
    static constexpr int32_t DMA_SHORTS_SHIFT   = 5;
    static constexpr int32_t DMA_SHORTS         = (1<<DMA_SHORTS_SHIFT);        // DMA 32 shorts
    static constexpr int32_t ANALOG_RAW_STORAGE_SHIFT = 15;
    static constexpr int32_t ANALOG_RAW_STORAGE = 1<<ANALOG_RAW_STORAGE_SHIFT;  // buffer is 32k shorts
    static constexpr int32_t DMA_BUFFERS        = 1<<(ANALOG_RAW_STORAGE_SHIFT - DMA_SHORTS_SHIFT);
    static constexpr int32_t ANALOG_RAW_MASK    = ((ANALOG_RAW_STORAGE) - 1);
    
    int16_t DMAbufferA[DMA_SHORTS];
    int16_t DMAbufferB[DMA_SHORTS];
    int16_t DCblocked [DMA_SHORTS];
    
    int16_t raw_buffer[ANALOG_RAW_STORAGE];
    
    public :
    
    void
    setLEDbrightness (const int32_t bright10 )
    {
        persistentBrightness = bright10;
    }

    int32_t init   (const int32_t sampleRate,
                    const int32_t gpio,
                    C_DMA_handler pDMAhandler );
        
    // reduce noise and current burn by turning LED off altogether after a few seconds
    int driveLED = 1;
    void    setDriveLED(int on) { driveLED = on; };
    
    void    deinit();
    
    int     start();
    void    stop();
    void    reset();
    
    int32_t overflow();
    
    int16_t *read  ( int16_t* buffer, const int32_t samples);
    void     write ( const int16_t *fromHere, const int32_t items );
    
    int32_t buffered();
    void    dma_handler();
    
    // this is an ugly hack to allow PIPPYGs to use the red LED where a ham-fisted change has broken the green one :(
    int32_t GPIOforLED = 25;
    
    void setGPIO(const int g) { GPIOforLED = g; };
};

#endif
