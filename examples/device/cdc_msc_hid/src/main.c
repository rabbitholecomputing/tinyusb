/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2018, hathach (tinyusb.org)
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
 * This file is part of the TinyUSB stack.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bsp/board.h"
#include "tusb.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTYPES
//--------------------------------------------------------------------+

/* Blink pattern
 * - 250 ms  : device not mounted
 * - 1000 ms : device mounted
 * - 2500 ms : device is suspended
 */
static uint32_t blink_interval_ms = 250;

void led_blinking_task(void);

extern void virtual_com_task(void);
extern void usb_hid_task(void);

/*------------- MAIN -------------*/
int main(void)
{
  board_init();

  tusb_init();

  while (1)
  {
    // tinyusb device task
    tud_task();

    led_blinking_task();

#if CFG_TUD_CDC
    virtual_com_task();
#endif

#if CFG_TUD_HID
    usb_hid_task();
#endif
  }

  return 0;
}

//--------------------------------------------------------------------+
// USB CDC
//--------------------------------------------------------------------+
#if CFG_TUD_CDC
void virtual_com_task(void)
{
  if ( tud_cdc_connected() )
  {
    // connected and there are data available
    if ( tud_cdc_available() )
    {
      uint8_t buf[64];

      // read and echo back
      uint32_t count = tud_cdc_read(buf, sizeof(buf));

      for(uint32_t i=0; i<count; i++)
      {
        tud_cdc_write_char(buf[i]);

        if ( buf[i] == '\r' ) tud_cdc_write_char('\n');
      }

      tud_cdc_write_flush();
    }
  }
}

// Invoked when cdc when line state changed e.g connected/disconnected
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
  (void) itf;

  // connected
  if ( dtr && rts )
  {
    // print initial message when connected
    tud_cdc_write_str("\r\nTinyUSB CDC MSC HID device example\r\n");
  }
}

// Invoked when CDC interface received data from host
void tud_cdc_rx_cb(uint8_t itf)
{
  (void) itf;
}

#endif

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+
#if CFG_TUD_HID
void usb_hid_task(void)
{
  // Poll every 10ms
  const uint32_t interval_ms = 10;
  static uint32_t start_ms = 0;

  if ( board_millis() < start_ms + interval_ms) return; // not enough time
  start_ms += interval_ms;

  uint32_t const btn = board_button_read();

  if ( tud_suspended() && btn )
  {
    // Wake up host if we are in suspend mode
    // and REMOTE_WAKEUP feature is enabled by host
    tud_remote_wakeup();
  }

#if 0
  /*------------- Keyboard -------------*/
  if ( tud_hid_keyboard_ready() )
  {
    if ( btn )
    {
      uint8_t keycode[6] = { 0 };

      for(uint8_t i=0; i < 6; i++)
      {
        if ( btn & (1 << i) ) keycode[i] = HID_KEY_A + i;
      }

      tud_hid_keyboard_keycode(0, keycode);
    }else
    {
      // Null means all zeroes keycodes
      tud_hid_keyboard_keycode(0, NULL);
    }
  }
#endif

#if 0
  /*------------- Mouse -------------*/
  if ( tud_hid_mouse_ready() )
  {
    enum { DELTA  = 5 };

    if ( btn & 0x01 ) tud_hid_mouse_move(-DELTA,      0); // left
    if ( btn & 0x02 ) tud_hid_mouse_move( DELTA,      0); // right
    if ( btn & 0x04 ) tud_hid_mouse_move(  0   , -DELTA); // up
    if ( btn & 0x08 ) tud_hid_mouse_move(  0   ,  DELTA); // down
  }
#endif
}

uint16_t tud_hid_generic_get_report_cb(uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
  // TODO not Implemented
  (void) report_id;
  (void) report_type;
  (void) buffer;
  (void) reqlen;

  return 0;
}

void tud_hid_generic_set_report_cb(uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
  // TODO not Implemented
  (void) report_id;
  (void) report_type;
  (void) buffer;
  (void) bufsize;
}

#endif

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
  blink_interval_ms = 1000;
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
  blink_interval_ms = 250;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void) remote_wakeup_en;
  blink_interval_ms = 2500;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
  blink_interval_ms = 1000;
}

//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+
void led_blinking_task(void)
{
  static uint32_t start_ms = 0;
  static bool led_state = false;

  // Blink every 1000 ms
  if ( board_millis() < start_ms + blink_interval_ms) return; // not enough time
  start_ms += blink_interval_ms;

  board_led_control(led_state);
  led_state = 1 - led_state; // toggle
}