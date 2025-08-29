/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Tons of additional stuff (c) Phil Atkin 2022-2024
 *
 * Seperated out high pass filter (dirkx, August 2025)
 */

#ifndef _PICO_HIGHPASS_FILTER_HPP_
#define _PICO_HIGHPASS_FILTER_HPP_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#if HPF_DEBUG
void setHPFcoeff ( int32_t c);
void dumbHPFbuffer ( short *outbuf,
                    const short *inbuf,
                    const int n );
#endif

void filterInit ( int32_t dBcut);

// this one is in use HPF followed by bandCut around 24kHz to suppress noisy microphone, 12-bits in, shorts out
void HPFbuffer (short *outp, const short *inp, int32_t n );

#endif
