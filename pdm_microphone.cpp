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

#include "filter.hpp"
#include "microphone.hpp"
#include "pdm_microphone.hpp"

pdm_microphone::pdm_microphone() {};
  
int pdm_microphone::start() { 
	return 0; 
};

int16_t *pdm_microphone::read( int16_t* buffer, const int32_t samples) { 
	return 0;
};
    
int32_t pdm_microphone::buffered() { 
	return 0;
};

void    pdm_microphone::dma_handler() {
};
