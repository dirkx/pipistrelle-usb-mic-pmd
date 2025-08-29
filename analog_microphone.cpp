/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 * 
 * Tons of additional stuff (c) Phil Atkin 2022-2024
 *
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

#include "analog_microphone.hpp"

#define FAKE_MIC 0

/*
 
 One final optimisation TODO - set up 2 chained DMAs using pipistrelle code
 
 */

int32_t
analog_microphone::buffered ()
{
    return (int32_t) (analog_wix - analog_rix);
}

int32_t
analog_microphone::overflow ()
{
    // reset on alarmingly close to full ...
    if (buffered () > (ANALOG_RAW_STORAGE - 1024)) return 1;
    
    return 0;
}

void
analog_microphone::reset ()
{
    // yes there will be a glitch if this happens
    analog_rix = analog_wix - (ANALOG_RAW_STORAGE>>1);
}

void
analog_microphone::LEDupdate(const short *p, const int32_t ix0, const int32_t itemShift )
{
    int32_t sum=0, v=0;
    
    for (int i=0;i<1<<itemShift;i++) {
        v = (int32_t) p[(ix0+i) & ANALOG_RAW_MASK];
        if (v < 0) v=-v;
        sum+=v;
    }
    
    // sum in range 0 .. 22 bits
    PWMLEDerror=PWMLEDerror + (sum>>(6+itemShift)) + (persistentBrightness >> 12); // nasty fine-tune!
    
    if (PWMLEDerror > 128) {
        PWMLEDerror-=128;
        gpio_put(GPIOforLED,1);
    }
    else {
        gpio_put(GPIOforLED,0);
    }
}

void analog_microphone::write ( const int16_t *fromHere, const int32_t itemShift )
{
    int32_t items = 1<<itemShift;
    // assumes items is always a multiple of DMA_SHORTS - in fact it should only ever be exactly DMA_SHORTS
    int32_t wix = 0xffffff & analog_wix;
    
    HPFbuffer (DCblocked, fromHere, items );
    
    for (int i=0;i<items;i++) {
        raw_buffer[(wix+i)&ANALOG_RAW_MASK] = DCblocked[i];
    }
    if (driveLED) LEDupdate(raw_buffer,wix,itemShift);
    
    // retain giant unmasked version of index
    analog_wix += items;
    
    buffersWritten++;
    
    // once we have been runnign smoothly for a second, reset so that rix is a half beffuer behind wix
    if (overflow() || (buffersWritten == 1024)) reset();
}

int16_t *
analog_microphone::read ( int16_t *toHere, int32_t items )
{
    if (readops == 0) {
        analog_rix = analog_wix - (ANALOG_RAW_STORAGE >> 1);
        readops    = 1;
    }
    
    int32_t rix = analog_rix;
    
    for (int i=0;i<items;i++) {
        toHere[i] = raw_buffer[(rix+i) & ANALOG_RAW_MASK];
    }
    
    // retain giant unmasked version of index
    analog_rix += items;

    return toHere;
}

int32_t
analog_microphone::init(const int32_t sampleRate,
                        const int32_t gpio,
                        C_DMA_handler pDMAhandler )
{
    DMAhandler = pDMAhandler;
    
    dma_channel = dma_claim_unused_channel(true);
    
    if (dma_channel < 0) {
        deinit();

        return -1;
    }

    float clk_div = (clock_get_hz(clk_adc) / ((float) sampleRate)) - 1;
    
    adc_set_round_robin(0);

    dma_channel_config dma_channel_cfg = dma_channel_get_default_config(dma_channel);

    channel_config_set_transfer_data_size(&dma_channel_cfg, DMA_SIZE_16);
    channel_config_set_read_increment    (&dma_channel_cfg, false);
    channel_config_set_write_increment   (&dma_channel_cfg, true);
    channel_config_set_dreq              (&dma_channel_cfg, DREQ_ADC);

    dma_irq = DMA_IRQ_0;

    dma_channel_configure(
                          dma_channel,
                          &dma_channel_cfg,
                          &raw_buffer[0],
                          &adc_hw->fifo,
                          DMA_SHORTS,
                          false
    );

    adc_gpio_init(gpio);
    adc_init();
    adc_select_input(gpio - 26);

    adc_fifo_setup(
        true,    // Write each completed conversion to the sample FIFO
        true,    // Enable DMA data request (DREQ)
        1,       // DREQ (and IRQ) asserted when at least 1 sample present
        false,   // We won't see the ERR bit because of 8 bit reads; disable.
        false    // Don't shift each sample to 8 bits when pushing to FIFO
    );

    adc_set_clkdiv(clk_div);
    
    return 0;
}

void 
analog_microphone::deinit()
{
    if (dma_channel > -1) {
        dma_channel_unclaim(dma_channel);

        dma_channel = -1;
    }
    readops    = 0;
}

int 
analog_microphone::start()
{
    readops    = 0;
    analog_wix = 0;
    analog_rix = 0;
    phase = 0;

    irq_set_enabled(dma_irq, true);
    irq_set_exclusive_handler(dma_irq, DMAhandler );

    if      (dma_irq == DMA_IRQ_0) dma_channel_set_irq0_enabled(dma_channel, true);
    else if (dma_irq == DMA_IRQ_1) dma_channel_set_irq1_enabled(dma_channel, true);
    else  return -1;
    
    dma_channel_transfer_to_buffer_now(
        dma_channel,
        DMAbufferA,
        DMA_SHORTS );

    adc_run(true); // start running the adc
    
    return 0;
}

void 
analog_microphone::stop()
{
    adc_run(false); // stop running the adc

    dma_channel_abort(dma_channel);

    if      (dma_irq == DMA_IRQ_0) dma_channel_set_irq0_enabled(dma_channel, false);
    else if (dma_irq == DMA_IRQ_1) dma_channel_set_irq1_enabled(dma_channel, false);

    irq_set_enabled(dma_irq, false);
}

void
analog_microphone::dma_handler()
{
    // clear IRQ
    if      (dma_irq == DMA_IRQ_0) dma_hw->ints0 = (1u << dma_channel);
    else if (dma_irq == DMA_IRQ_1) dma_hw->ints1 = (1u << dma_channel);
    
    // give the channel a new buffer to write to and re-trigger it
    short *buffer = phase ? &DMAbufferA[0] : &DMAbufferB[0];
    dma_channel_transfer_to_buffer_now(
                                       dma_channel,
                                       buffer,
                                       DMA_SHORTS );
    
    phase ^= 1;
    
    // now copy what we just DMAd in
    buffer = phase ? &DMAbufferA[0] : &DMAbufferB[0];
    
    write   ( buffer, DMA_SHORTS_SHIFT );
}
