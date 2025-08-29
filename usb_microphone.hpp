/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 * 
 */

#ifndef _USB_MICROPHONE_HPP_
#define _USB_MICROPHONE_HPP_

#include <stdint.h>

#include "usb_micdefs.h"

extern "C" void usb_microphone_init();
extern "C" void usb_microphone_set_tx_ready_handler(usb_microphone_tx_ready_handler_t handler);
extern "C" void usb_microphone_task();
extern "C" uint16_t usb_microphone_write(const void * data, uint16_t len);

#endif
