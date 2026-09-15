## Smooth Scrolling Mouse

<img src="images\main-image.jpg" style="zoom:100%;" />

#### Overview

In this project, I built a a new type of USB computer mouse capable of scrolling and panning smoothly in any direction. The scroll wheel is replaced with a finger-operated joystick, which is held static during usage instead of being constantly when navigating large documents and workspaces.

Firmware was developed for a STM32 development board, involving writing custom USB device applications, a driver for a PMW3610 optical sensor, and reverse-engineering the requirements of Windows hosts using protocol knowledge and Wireshark. Furthermore, a chassis with clicking mechanism was designed using Autodesk Fusion and fabricated using an FDM 3D printer.

#### System Design

The system was designed using a [WeAct STM32F411CEU6 development board](https://stm32-base.org/boards/STM32F411CEU6-WeAct-Black-Pill-V2.0.html), chosen for its small size, user USB connector, and USB FS peripheral supported by TinyUSB. A simple breakout board-based system architecture was used given that this was a quick prototype without large SI constraints.

The MCU development board was soldered onto perfboard along with Kailh silent mouse switches, pull-down resistors, and pin headers to form a mainboard. A PMW3610-DM optical mouse sensor breakout board was used for cursor tracking. Lastly, scrolling was implemented using a joystick.

The joystick chosen was a replacement for the Nintendo switch joystick and which was chosen because of its small size, low force required to displace, and built-in button. Additionally, an FPC connector breakout board was used to interface with it.

<img src="images\inside.jpg" style="zoom:100%;" />

#### Firmware Development

<img src="images\app-pinout.png" style="zoom:60%;" />

##### Main Program

The main loop is structured as a single non-blocking cycle with one deliberate blocking point: a fixed 10-ms minimum loop time enforced right before the report is sent, which keeps the report rate steady regardless of how long sensor polling and other input handling take. At the top of every iteration, a timestamp is taken and the report's `x` and `y` fields are cleared, since these are relative values that should default to zero unless new motion is detected that iteration (unlike the buttons, which reflect a continuous state).

Pseudocode of the main program loop follows:

```pseudocode
loop:
    timestamp = now()
    report.x = 0
    report.y = 0

    tud_task()                      // service USB stack

    if MOT_pin is asserted:
        read burst motion data from PMW3610
        if valid:
            report.x = convert(delta_x)
            report.y = convert(delta_y)

    report.left_button   = !MB1_pin
    report.right_button  = !MB2_pin
    report.middle_button = !MB3_pin

    start ADC conversion (channel 1)
    report.scroll = convert(adc_value)

    start ADC conversion (channel 2)
    report.pan = convert(adc_value)
    stop ADC

    wait until (now() - timestamp) >= 10ms
    send report
```

The optical sensor is polled and the `MOT` pin is just read as a plain GPIO input each iteration. Updates to mouse buttons, however, are implemented using interrupts.

The joystick's two axes go through ADC1, configured for two channels in discontinuous conversion mode with a single conversion per trigger (`NbrOfDiscConversion = 1`). Rather than firing off both channels back-to-back in scan mode, each `HAL_ADC_Start()`/`HAL_ADC_PollForConversion()` call advances the sequencer by exactly one channel, so the two axes can be read as two distinct, independently-timed conversions within the loop. No DMA is used as polling was sufficiently fast at this conversion rate.

With respect to clock configuration, SYSCLK is set to 16 MHz and PLL M/N/Q dividers were chosen to provide exactly 48 MHz to the USB OTG peripheral.

##### PMW3610 Driver

A driver for the PMW3610 optical sensor is implemented in `inc/pmw3610.h`. The PMW3610 communicates over a 3-wire SPI-like interface, so SPI2 is configured half-duplex, `SPI_DIRECTION_1LINE`, meaning MOSI and MISO share a single physical line and the peripheral is manually switched between transmit and receive around each register access. It's set to Mode 0 (`CLKPolarity = LOW`, `CLKPhase = 1EDGE`) and a baud rate of 31.25 kbit. For simplicity, chip-select is implemented with GPIO calls.

Every register access follows the same two-phase pattern: select the chip, send a one-byte header with the MSB indicating read (0) or write (1) and the lower 7 bits as the register address, then either transmit or receive the data byte(s), and deselect. A read looks like this:

```c
HAL_GPIO_WritePin(dev_p->NCS_port_p, dev_p->NCS_pin, NCS_SELECT);

uint8_t first_byte[1] = {address & 0b01111111};  // MSB = 0: read operation
result = HAL_SPI_Transmit(dev_p->hspi_p, first_byte, 1, SPI_DEFAULT_TIMEOUT);

result = HAL_SPI_Receive(dev_p->hspi_p, data_p, size, SPI_DEFAULT_TIMEOUT);

HAL_GPIO_WritePin(dev_p->NCS_port_p, dev_p->NCS_pin, NCS_DESELECT);
```

Writes follow a similar process, sending the address with the MSB set to 1 followed by the value byte. `write_reg()` additionally wraps this in a clock-enable/disable sequence (writing specific values to the sensor's clock control register before and after) since the datasheet requires the sensor's internal SPI clock to be explicitly turned on before a write and off afterward.

`pmw3610_init()` runs through the sensor's documented startup sequence: power the sensor up from shutdown, clear the observation register, then read it back and check that the lower four bits read `1111` as the datasheet specifies (this confirms the sensor booted correctly). It then reads the product ID register and checks it against the expected value for the PMW3610, and finally writes a small set of performance/downshift registers to their recommended initial values.

`pmw3610_get_values()` does a single 4-byte burst read starting at the burst register, which returns the motion status byte plus the low bytes of X and Y and a shared high-nibble byte. If the motion bit isn't set, there's no new data and the function returns early with `valid = PMW3610_MOT_REG_INVAL`. Otherwise, the delta values need to be reconstructed: X and Y are each 12-bit signed integers, but the sensor packs their high 4 bits together into a single byte (upper nibble for X, lower nibble for Y) alongside their separate low bytes. The driver shifts each nibble into position, ORs it together with the corresponding low byte to get a 12-bit value, then manually sign-extends from bit 11 up through bit 15 before casting to `int16_t`.

##### USB Device Application

While other software methods as well as other firmware methods were evaluated (such as by emulating a Windows Precision touchpad; [repository link here](https://github.com/justinjradi/stm32-usb-touchpad)), I decided to use the [specification for smooth scrolling mice on Windows](https://download.microsoft.com/download/b/d/1/bd1f7ef4-7d72-419e-bc5c-9f79ad7bb66e/wheel.docx) in my latest prototype.

TinyUSB (version 0.17.0), was used to implement the HID-class USB device and is configured in `inc/tusb_config.h` for USB FS. The HID report descriptors that are used to implement the host's requirements for a HID smooth scrolling mouse follow. They are contained in `src/usb_descriptors.c`:

```c
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
	0x81, 0x02,        //       Input (Data,Var,Abs)
	0x95, 0x01,        //       Report Count (1)
	0x75, 0x03,        //       Report Size (3)
	0x81, 0x01,        //       Input (Const,Array,Abs)      -- 3-bit padding after 5 buttons
	0x05, 0x01,        //       Usage Page (Generic Desktop Ctrls)
	0x09, 0x30,        //       Usage (X)
	0x09, 0x31,        //       Usage (Y)
	0x95, 0x02,        //       Report Count (2)
	0x75, 0x08,        //       Report Size (8)
	0x15, 0x81,        //       Logical Minimum (-127)
	0x25, 0x7f,        //       Logical Maximum (127)
	0x81, 0x06,        //       Input (Data,Var,Rel)
	0xA1, 0x02,        //       Collection (Logical)
	0x85, REPORTID_RES_MULTIPLIER, //  Report ID (REPORTID_RES_MULTIPLIER)
	0x09, 0x48,        //         Usage (Resolution Multiplier)
	0x95, 0x01,        //         Report Count (1)
	0x75, 0x02,        //         Report Size (2)
	0x15, 0x00,        //         Logical Minimum (0)
	0x25, 0x01,        //         Logical Maximum (1)
	0x35, 0x01,        //         Physical Minimum (1)
	0x45, MAX_SCROLL_RES,   //     Physical Maximum (MAX_SCROLL_RES)
	0xB1, 0x02,        //         Feature (Data,Var,Abs,Non-volatile)
	0x85, REPORTID_MOUSE,  //     Report ID (REPORTID_MOUSE)
	0x09, 0x38,        //         Usage (Wheel)
	0x35, 0x00,        //         Physical Minimum (0)
	0x45, 0x00,        //         Physical Maximum (0)
	0x95, 0x01,        //         Report Count (1)
	0x75, 0x08,        //         Report Size (8)
	0x15, 0x81,        //         Logical Minimum (-127)
	0x25, 0x7f,        //         Logical Maximum (127)
	0x81, 0x06,        //         Input (Data,Var,Rel)
	0xC0,              //       End Collection
	0xA1, 0x02,        //       Collection (Logical)
	0x85, REPORTID_RES_MULTIPLIER, //  Report ID (REPORTID_RES_MULTIPLIER)
	0x09, 0x48,        //         Usage (0x48)
	0x95, 0x01,        //         Report Count (1)
	0x75, 0x02,        //         Report Size (2)
	0x15, 0x00,        //         Logical Minimum (0)
	0x25, 0x01,        //         Logical Maximum (1)
	0x35, 0x01,        //         Physical Minimum (1)
	0x45, MAX_PAN_RES,      //     Physical Maximum (MAX_PAN_RES)
	0xB1, 0x02,        //         Feature (Data,Var,Abs,Non-volatile)
	0x35, 0x00,        //         Physical Minimum (0)
	0x45, 0x00,        //         Physical Maximum (0)
	0x75, 0x04,        //         Report Size (4)
	0xB1, 0x01,        //         Feature (Const,Array,Abs,Non-volatile)  -- padding
	0x85, 0x01,        //         Report ID (1)
	0x05, 0x0C,        //         Usage Page (Consumer)
	0x0A, 0x38, 0x02,  //         Usage (AC Pan)
	0x95, 0x01,        //         Report Count (1)
	0x75, 0x08,        //         Report Size (8)
	0x15, 0x81,        //         Logical Minimum (-127)
	0x25, 0x7f,        //         Logical Maximum (127)
	0x81, 0x06,        //         Input (Data,Var,Rel)
	0xC0,              //       End Collection
	0xC0,              //     End Collection
	0xC0,              //   End Collection
	0xC0               // End Collection
};
```

The descriptor nests two logical collections inside the mouse's physical collection: one for scrolling and one for panning with having its own resolution multiplier feature item under `REPORTID_RES_MULTIPLIER`. The implementation of a library for the device application is contained in `inc/evcm.h` and it defines the following structs for reports:

```c
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
```

The mouse input report is sent from the device to host using the `tud_hid_report()` function whenever the function `evcm_send_mouse_report()` is called from the main loop.

To handle the resolution multiplier feature report being sent from the host to device, the following line is added to the `tud_hid_set_report_cb()` callback:

```c
tud_hid_report(0, buffer, bufsize);
```

Additionally, the resolution multiplier feature report is sent to the host whenever it requests it. This was not well-documented and discovered by analyzing USB traffic. Furthermore, TinyUSB didn't support a method for users of the library to implement device-to-host feature reporting at the time, so I modified `hidd_control_xfer_cb()` in `tinyusb/src/class/hid/hid_device.c` of the source code to implement this. A snippet from the function showing the modification is shown:

```c
case HID_REQ_CONTROL_GET_REPORT:
  if (stage == CONTROL_STAGE_SETUP) {
    uint8_t const report_type = tu_u16_high(request->wValue);
    uint8_t const report_id = tu_u16_low(request->wValue);

    uint8_t *report_buf = p_hid->ctrl_buf;
    uint16_t req_len = tu_min16(request->wLength, CFG_TUD_HID_EP_BUFSIZE);

    uint16_t xferlen = 0;

    // If host request a specific Report ID, add ID to as 1 byte of response
    if ((report_id != HID_REPORT_TYPE_INVALID) && (req_len > 1)) {
      *report_buf++ = report_id;
      req_len--;

      xferlen++;

      /*
       * Respond to get_report over endpoint 0
       */
      if (report_id == REPORTID_RES_MULTIPLIER)
      {
        *report_buf++ = 0x05;  // Data byte
        xferlen++;
      }
      /*
       *
       */
    }

    xferlen += tud_hid_get_report_cb(hid_itf, report_id, (hid_report_type_t)report_type, report_buf, req_len);
    TU_ASSERT(xferlen > 0);

    tud_control_xfer(rhport, request, p_hid->ctrl_buf, xferlen);
  }
  break;
```

#### Chassis Design

A chassis was designed in Autodesk Fusion, including a base plate and bracket to hold the joystick (seen in pictures in System Design section), and a shell. A cross section of the mechanism for pressing the mainboard buttons, which was part of the shell, is shown below. Models for the mainboard and optical sensor breakout board were also created to aid with space planning. The CAD file, `mouse-body-v115.f3z` is available for download.

<img src="images\clicking-mechanism.png" style="zoom:60%;" />