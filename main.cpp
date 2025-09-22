/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This example previously created a USB Microphone device using the TinyUSB
 * library and captures data from a PDM microphone using a sample
 * rate of 16 kHz, to be sent the to PC.
 *
 * The USB microphone code is based on the TinyUSB audio_test example.
 *
 * https://github.com/hathach/tinyusb/tree/master/examples/device/audio_test
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/platform.h"
#include "pico/sync.h"

#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "hardware/dma.h"
#include "hardware/sync.h"
#include "hardware/pll.h"
#include "hardware/clocks.h"
#include "hardware/watchdog.h"
#include "tusb.h"

#include "microphone.hpp"
#include "analog_microphone.hpp"
#include "pdm_microphone.hpp"

microphone *_microphone = 0;

extern "C" void DMAhandler();


#if MICROPHONE == PDM
#define GPIO_DAT (12) // must be in base 0
#define GPIO_CLK (13) // must be clock capable
#endif

#if MICROPHONE == ANALOG
#define GPIO_AD_MIC (26)
#endif

#if MICROPHONE == ANALOG
void DMAhandler ()
{
    if (_microphone) 
	((analog_microphone*)_microphone)->dma_handler();
}
#elif MICROPHONE == PDM
void DMAhandler (uint8_t *buffer, size_t len)
{
    if (_microphone) 
	((pdm_microphone*)_microphone)->dma_handler(buffer,len);
}
#endif

int32_t gpios[11];

int32_t LED_init ()
{
    static constexpr int32_t UNFLIP_GPIOS [11] = { 11,10,8,7,6,5,4,3,2,1, 25};
    static constexpr int32_t FLIP_GPIOS   [11] = { 1,2,3,4,5,6,7,8,10,11, 25};

    for (int i=0;i<11;i++) {
        gpios[i] = FLIP_GPIOS[i];
        
        gpio_init    (gpios[i]);
        gpio_set_dir (gpios[i], GPIO_OUT);
    }
    
    // 9 for red, 10 for green
    return FLIP_GPIOS[10]; // [9];
}

void LEDpattern (int v)
{
    for (int i=0;i<10;i++) {
        gpio_put(gpios[i], v&1);
        v>>=1;
    }
}

#include "usb_microphone.hpp"

// callback functions
extern "C" void on_usb_microphone_tx_ready();

int
setCPUclock(const int32_t kHz)
{
    uint vco_freq;
    uint post_div1;
    uint post_div2;
    
    printf ( "setCPUclock : kHz %d\n", kHz );
    
    if (check_sys_clock_khz(kHz, &vco_freq, &post_div1, &post_div2)) {
        printf ( "check_sys_clock_khz : PASSED : kHz %d\n", kHz );
        clock_configure(clk_sys,
                        CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX,
                        CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB,
                        48 * MHZ,
                        48 * MHZ);
        
        pll_init(pll_sys, 1, vco_freq, post_div1, post_div2);
        uint32_t freq = vco_freq / (post_div1 * post_div2);
        
        // Configure clocks
        // CLK_REF = XOSC (12MHz) / 1 = 12MHz
        clock_configure(clk_ref,
                        CLOCKS_CLK_REF_CTRL_SRC_VALUE_XOSC_CLKSRC,
                        0, // No aux mux
                        12 * MHZ,
                        12 * MHZ);
        
        clock_configure(clk_sys,
                        CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX,
                        CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS,
                        freq, freq);
        
        // DO NOT SET PERIPHERAL CLOCK - gives me more SPI flexibility
        return 1;
    }
    else {
        printf ( "check_sys_clock_khz : FAILED : kHz %d\n", kHz );
        return 0;
    }
}
    
void
PSUquiet ( const int quiet )
{
    if (quiet) gpio_put(23, 1 );
    else       gpio_put(23, 0 );
    sleep_ms (3);
}

void failWithLEDs ( const int32_t pattern )
{
    LEDpattern(pattern);
    while (1) {
        sleep_ms(50);
    }
}

// Callback from TinyUSB library when a new mic buffer is needed

static short USBbuffer[512];

void on_usb_microphone_tx_ready()
{
    while (_microphone->buffered() < 384)
        ;
    usb_microphone_write ( _microphone->read ( USBbuffer, 384 ), 384 * sizeof(short));
}

int32_t BIRDCOEFF  = 1;
int32_t BATCOEFF   = 4096;

int32_t HPFCOEFF = BATCOEFF;

extern void blastGreenLED();


void Core1()
{
    uint64_t time0 = time_us_64();
    int32_t secs = 0;
    
    int32_t LEDbrightness = 1024;
    
    // flash 10 times in 5 seconds
    while (secs < 10) {
        uint64_t t = time_us_64();
        int32_t dt = t-time0;
        
        // dt in range 0 .. 500000
        int32_t bright = (250000 - dt);
        if (bright <    0) bright = 0;
        
        // boost by 10 / 4
        _microphone->setLEDbrightness((bright * 10) >> 2);
        
        if (dt > 500000) {
            time0+=500000;
            secs++;
        }
    }
    
    // leave LED on for total 30 seconds
    while (secs < 30) {
        uint64_t t = time_us_64();
        int32_t dt = t-time0;
        
        secs = dt >> 20;
    }
    
    // turn LED off to minimize generated noise
    _microphone->setDriveLED(0);
    
    while (1) {
        sleep_ms(100000);
    }
}

int main(void)
{
    int32_t iter = 0;

    setCPUclock(108000); // seems to minimize energy bands
    
    int32_t useLED = LED_init();
    LEDpattern(0x0);

    PSUquiet     (1);

#if MICROPHONE == ANALOG
    gpio_init    (23);
    gpio_set_dir (23, GPIO_OUT);

    _microphone = (microphone *) new analog_microphone;
#elif MICROPHONE == PDM
    _microphone = (microphone*) new pdm_microphone(384000,GPIO_DAT,GPIO_CLK,&DMAhandler);
#else
    #error "No microphone defined."
#endif

    // use LED pattern o diagnose failure modes - won't work on PIPPYG!
    if (_microphone == 0) failWithLEDs (0xaaaa);

    _microphone->setDriveLED(1);
    _microphone->setGPIO(useLED);
    
#if MICROPHONE == ANALOG
    if (((analog_microphone*)_microphone)->init(384000,28,&DMAhandler) < 0) failWithLEDs (0xcccc);
#endif

#ifdef HPF_DEBUG
    _microphone->setHPFcoeff(BATCOEFF);
#endif
    
    if (_microphone->start() < 0) failWithLEDs (0xf0f0);

    usb_microphone_init();
    usb_microphone_set_tx_ready_handler(on_usb_microphone_tx_ready);
    
    multicore_launch_core1(&Core1 );
    
    while (1) {
        usb_microphone_task();
    }
    
    return 0;
}
