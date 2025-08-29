/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Tons of additional stuff (c) Phil Atkin 2022-2024
 */

#ifndef _PICO_ANALOG_MICROPHONE_HPP_
#define _PICO_ANALOG_MICROPHONE_HPP_

class analog_microphone {
    private :
    
    // for the startup pulses
    int32_t persistentBrightness = 0;
    
    typedef void (*C_DMA_handler)();
    
    int32_t delayv = 0;
    
    // don't ask, I can't remember how or why this works ... dick with this at your peril
    enum {
        COEFFBITS     = 13,
        BAT_SHIFT     = 4,
        BIRD_SHIFT    = 12,
        FRACBITS      =  28,
        FSCALE        = (1<<COEFFBITS),
        BIQUADSHIFT   = 4,
        BIQUADSUB     = 2048,
        AVOIDCLIPPING = ((1 << (FRACBITS - 12)) * 15) >> 4
    };
    
    int32_t PWMLEDvalue = 0;
    int32_t PWMLEDerror = 0;

    short nextWobulatedSample ();

    void LEDupdate(const short *p, const int32_t ix0, const int32_t items );

    void wobulate ( const int16_t *fromHere, const int32_t items );

    void
    filterInit ( int32_t dBcut)
    {
        float a2,b0,b1,b2;
        
        switch (dBcut) {
            case 12 :
                a2 =  0.72322786f;
                b0 =  0.91071516f;
                b1 = -1.59205495f;
                b2 =  0.81251270f;
                break;
            case 32 :
                a2 =  0.98418098f;
                b0 =  0.99209049f;
                b1 = -1.83314419f;
                b2 =  0.99209049f;
                break;
            case 6 :
                // -6
                a2 =  0.93463886;
                b0 =  0.98369852;
                b1 = -1.78737325;
                b2 =  0.95094035;
                break;
                
            case 9:
                // -9dB
                a2 =  0.92279376;
                b0 =  0.97509378;
                b1 = -1.77642980;
                b2 =  0.94769998;
                
                break;
                
            case 3 :
            default :
                // -3
                a2 =  0.94471892;
                b0 =  0.99192746;
                b1 = -1.79668601;
                b2 =  0.95279146;
                
                break;
        }
        
        kia2 = (int32_t) (FSCALE * a2);
        kib0 = (int32_t) (FSCALE * b0);
        kib1 = (int32_t) (FSCALE * b1);
        kib2 = (int32_t) (FSCALE * b2);
        
    };
    
    int32_t buffersWritten = 0;
    
    int32_t d16  = 0;
    int32_t dFIX = 0;
    
    int32_t ix1=0,ix2=0,iy1=0,iy2=0;
    int32_t kia2,kib0,kib1,kib2;
    
    inline int32_t
    clampto16 ( int32_t v )
    {
        if      (v < -32767) return -32767;
        else if (v >  32767) return  32767;
        else return v;
    };
    
    inline int32_t stepHPFtoshort15 ( int32_t v, int32_t shift)
    {
        int32_t outFIX = ((v-2048)<<(FRACBITS - 12)) - dFIX;
        dFIX   +=(outFIX >> shift);
        outFIX = outFIX >> (FRACBITS-16);
        return outFIX;
    };
    
    inline int32_t stepBiQuadOPT_ShortShort (const int32_t HPFvalue)
    {
        int32_t ix = HPFvalue;
        int32_t iy  = ((kib0*ix) + (kib1*(ix1-iy1)) + (kib2*ix2) - (kia2*iy2)) >> COEFFBITS;
        
        ix2=ix1;
        ix1=ix;
        iy2=iy1;
        iy1=iy;
        
        return iy;
    };
    
    
    // this one is in use
    // HPF followed by bandCut around 24kHz to suppress noisy microphone, 12-bits in, shorts out
    void
    HPFbuffer (short *outp, const short *inp, int32_t n )
    {
        for (int i=0;i<n;i++ ){
            outp[i] = clampto16(stepBiQuadOPT_ShortShort(stepHPFtoshort15 ( inp[i], BAT_SHIFT)));
        }
    }
    
    // simplest of all DC blockers / HPF
    // the smaller this is, the more low frequency energy is passed
    
    int32_t HPFCOEFF = 32;
    
    void dumbHPFbuffer ( short *outbuf,
                        const short *inbuf,
                        const int n )
    {
        int32_t outv, inv, delv;
        
        delv = delayv;
        
        for (int i=0;i<n;i++) {
            inv  = (inbuf[i] - 2048) << 4;
            outv = inv - (delv >> 14);
            outbuf[i] = outv;
            delv += (outv * HPFCOEFF);
            // outbuf[i] = inv;
        }
        
        delayv = delv;
    }
    
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
    
    void setHPFcoeff ( int32_t c)
    {
        // just range check them reasonably
        if (c < 3)    c=3;
        if (c > 8000) c=8000;
        
        HPFCOEFF = c;
    };
    
    analog_microphone()
    {
        filterInit (12);
    }
    
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
