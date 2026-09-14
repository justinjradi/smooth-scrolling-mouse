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

#include "board_api.h"
#include "tusb.h"

/* A combination of interfaces must have a unique product id, since PC will save device driver after the first plug.
 * Same VID/PID with different interface e.g MSC (first), then CDC (later) will possibly cause system error on PC.
 *
 * Auto ProductID layout's Bitmap:
 *   [MSB]         HID | MSC | CDC          [LSB]
 */
#define _PID_MAP(itf, n)  ( (CFG_TUD_##itf) << (n) )
#define USB_PID           (0x4000 | _PID_MAP(CDC, 0) | _PID_MAP(MSC, 1) | _PID_MAP(HID, 2) | \
                           _PID_MAP(MIDI, 3) | _PID_MAP(VENDOR, 4) )

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+
tusb_desc_device_t const desc_device =
{
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = 0xCafe,
    .idProduct          = USB_PID,
    .bcdDevice          = 0x0100,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,

    .bNumConfigurations = 0x01
};

// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
uint8_t const * tud_descriptor_device_cb(void)
{
  return (uint8_t const *) &desc_device;
}

//--------------------------------------------------------------------+
// HID Report Descriptor
//--------------------------------------------------------------------+

uint8_t const desc_hid_report[] =
{
			0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
			0x09, 0x02,        // Usage (Mouse)
			0xA1, 0x01,        // Collection (Application)
			0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
			0x09, 0x02,        //   Usage (Mouse)
			0xA1, 0x02,        //   Collection (Logical)
			0x85, REPORTID_MOUSE,  //     Report ID (REPORTID_MOUSE)
			0x09, 0x01,        //     Usage (Pointer)
			0xA1, 0x00,        //     Collection (Physical)
			0x05, 0x09,        //       Usage Page (Button)
			0x19, 0x01,        //       Usage Minimum (0x01)
			0x29, 0x05,        //       Usage Maximum (0x05)
			0x15, 0x00,        //       Logical Minimum (0)
			0x25, 0x01,        //       Logical Maximum (1)
			0x95, 0x05,        //       Report Count (5)
			0x75, 0x01,        //       Report Size (1)
			0x81, 0x02,        //       Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
			0x95, 0x01,        //       Report Count (1)
			0x75, 0x03,        //       Report Size (3)
			0x81, 0x01,        //       Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
			0x05, 0x01,        //       Usage Page (Generic Desktop Ctrls)
			0x09, 0x30,        //       Usage (X)
			0x09, 0x31,        //       Usage (Y)
			0x95, 0x02,        //       Report Count (2)
	    0x75, 0x08,                    //       REPORT_SIZE (8)
	    0x15, 0x81,                    //       LOGICAL_MINIMUM (-127)
	    0x25, 0x7f,                    //       LOGICAL_MAXIMUM (127)
			0x81, 0x06,        //       Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
			0xA1, 0x02,        //       Collection (Logical)
			0x85, REPORTID_RES_MULTIPLIER, //         Report ID (REPORTID_RES_MULTIPLIER)
			0x09, 0x48,        //         Usage (Resolution Multiplier)
			0x95, 0x01,        //         Report Count (1)
			0x75, 0x02,        //         Report Size (2)
			0x15, 0x00,        //         Logical Minimum (0)
			0x25, 0x01,        //         Logical Maximum (1)
			0x35, 0x01,        //         Physical Minimum (1)
			0x45, MAX_SCROLL_RES,   //         Physical Maximum (MAX_SCROLL_RES)
			0xB1, 0x02,        //         Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
			0x85, REPORTID_MOUSE,  //         Report ID (REPORTID_MOUSE)
			0x09, 0x38,        //         Usage (Wheel)
			0x35, 0x00,        //         Physical Minimum (0)
			0x45, 0x00,        //         Physical Maximum (0)
			0x95, 0x01,        //         Report Count (1)
	    0x75, 0x08,                    //       REPORT_SIZE (8)
	    0x15, 0x81,                    //       LOGICAL_MINIMUM (-127)
	    0x25, 0x7f,                    //       LOGICAL_MAXIMUM (127)
			0x81, 0x06,        //         Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
			0xC0,              //       End Collection
			0xA1, 0x02,        //       Collection (Logical)
			0x85, REPORTID_RES_MULTIPLIER, //         Report ID (REPORTID_RES_MULTIPLIER)
			0x09, 0x48,        //         Usage (0x48)
			0x95, 0x01,        //         Report Count (1)
			0x75, 0x02,        //         Report Size (2)
			0x15, 0x00,        //         Logical Minimum (0)
			0x25, 0x01,        //         Logical Maximum (1)
			0x35, 0x01,        //         Physical Minimum (1)
			0x45, MAX_PAN_RES,        //         Physical Maximum (MAX_PAN_RES)
			0xB1, 0x02,        //         Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
			0x35, 0x00,        //         Physical Minimum (0)
			0x45, 0x00,        //         Physical Maximum (0)
			0x75, 0x04,        //         Report Size (4)
			0xB1, 0x01,        //         Feature (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
			0x85, 0x01,        //         Report ID (1)
			0x05, 0x0C,        //         Usage Page (Consumer)
			0x0A, 0x38, 0x02,  //         Usage (AC Pan)
			0x95, 0x01,        //         Report Count (1)
	    0x75, 0x08,                    //       REPORT_SIZE (8)
	    0x15, 0x81,                    //       LOGICAL_MINIMUM (-127)
	    0x25, 0x7f,                    //       LOGICAL_MAXIMUM (127)
			0x81, 0x06,        //         Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
			0xC0,              //       End Collection
			0xC0,              //     End Collection
			0xC0,              //   End Collection
	  0xC0								// End collection
};

// Invoked when received GET HID REPORT DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const * tud_hid_descriptor_report_cb(uint8_t itf)
{
  (void) itf;
  return desc_hid_report;
}

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

enum
{
  ITF_NUM_HID,
  ITF_NUM_TOTAL
};

#define  CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_HID_INOUT_DESC_LEN)

#define EPNUM_HID   0x01

uint8_t const desc_configuration[] =
{
  // Config number, interface count, string index, total length, attribute, power in mA
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),

  // Interface number, string index, protocol, report descriptor len, EP Out & In address, size & polling interval
  TUD_HID_INOUT_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report), EPNUM_HID, 0x80 | EPNUM_HID, CFG_TUD_HID_EP_BUFSIZE, 10)
};

// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const * tud_descriptor_configuration_cb(uint8_t index)
{
  (void) index; // for multiple configurations
  return desc_configuration;
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

// String Descriptor Index
enum {
  STRID_LANGID = 0,
  STRID_MANUFACTURER,
  STRID_PRODUCT,
  STRID_SERIAL,
};

// array of pointer to string descriptors
char const *string_desc_arr[] =
{
  (const char[]) { 0x09, 0x04 }, // 0: is supported language is English (0x0409)
  "TinyUSB",                     // 1: Manufacturer
  "TinyUSB Device",              // 2: Product
  NULL,                          // 3: Serials will use unique ID if possible
};

static uint16_t _desc_str[32 + 1];

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  (void) langid;
  size_t chr_count;

  switch ( index ) {
    case STRID_LANGID:
      memcpy(&_desc_str[1], string_desc_arr[0], 2);
      chr_count = 1;
      break;

    case STRID_SERIAL:
      chr_count = board_usb_get_serial(_desc_str + 1, 32);
      break;

    default:
      // Note: the 0xEE index string is a Microsoft OS 1.0 Descriptors.
      // https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors

      if ( !(index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0])) ) return NULL;

      const char *str = string_desc_arr[index];

      // Cap at max char
      chr_count = strlen(str);
      size_t const max_count = sizeof(_desc_str) / sizeof(_desc_str[0]) - 1; // -1 for string type
      if ( chr_count > max_count ) chr_count = max_count;

      // Convert ASCII string into UTF-16
      for ( size_t i = 0; i < chr_count; i++ ) {
        _desc_str[1 + i] = str[i];
      }
      break;
  }

  // first byte is length (including header), second byte is string type
  _desc_str[0] = (uint16_t) ((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));

  return _desc_str;
}
