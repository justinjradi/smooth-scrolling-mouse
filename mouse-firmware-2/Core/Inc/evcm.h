/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tusb.h"

//--------------------------------------------------------------------+
// User report typedefs
//--------------------------------------------------------------------+

// Mouse Input Report
typedef struct __attribute__((packed))
{
	uint8_t left_button : 1;
	uint8_t right_button : 1;
	uint8_t middle_button : 1;
	uint8_t back_button : 1;
	uint8_t forward_button : 1;
	uint8_t padding : 3;
	int8_t x;
	int8_t y;
	int8_t scroll;
	int8_t pan;
} MouseReport;

// Resolution Multiplier Feature Report
typedef struct __attribute__((packed))
{
	uint8_t supports_scroll_res : 2;
	uint8_t supports_pan_res : 2;
	uint8_t padding : 4;
} ResMultiplierReport;

//--------------------------------------------------------------------+
// User function prototypes
//--------------------------------------------------------------------+

void evcm_send_mouse_report(MouseReport report);

//--------------------------------------------------------------------+
// Device callback prototypes
//--------------------------------------------------------------------+

void tud_mount_cb(void);
void tud_umount_cb(void);
void tud_suspend_cb(bool remote_wakeup_en);
void tud_resume_cb(void);

//--------------------------------------------------------------------+
// USB HID function prototypes
//--------------------------------------------------------------------+

uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen);
void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize);
