#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "bsp/board_api.h"
#include "tusb.h"
#include "class/cdc/cdc.h" // Add this line to include the definition of cdc_line_coding_t
#include "lwjson/lwjson.h"

/* LwJSON instance and tokens */

static lwjson_token_t tokens[128];
static lwjson_t lwjson;

/* Blink pattern
 * - 250 ms  : device not mounted
 * - 1000 ms : device mounted
 * - 2500 ms : device is suspended
 */
enum
{
  BLINK_NOT_MOUNTED = 250,
  BLINK_MOUNTED = 1000,
  BLINK_SUSPENDED = 2500,
};

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

static void led_blinking_task(void);
static void cdc_task(void);

/*------------- MAIN -------------*/
int main(void)
{
  board_init();

  // init device stack on configured roothub port
  tusb_rhport_init_t dev_init = {
      .role = TUSB_ROLE_DEVICE,
      .speed = TUSB_SPEED_AUTO};
  tusb_init(BOARD_TUD_RHPORT, &dev_init);

  if (board_init_after_tusb)
  {
    board_init_after_tusb();
  }

  // lwjson_init(&lwjson, tokens, LWJSON_ARRAYSIZE(tokens));
  // if (lwjson_parse(&lwjson, "{\"mykey\":\"myvalue\"}") == lwjsonOK)
  // {

  //   const lwjson_token_t *t;

  //   /* Find custom key in JSON */

  //   if ((t = lwjson_find(&lwjson, "mykey")) != NULL)
  //   {
  //   }

  //   /* Call this when not used anymore */

  //   lwjson_free(&lwjson);
  // }

  while (1)
  {
    tud_task(); // tinyusb device task
    cdc_task();
    led_blinking_task();
    // board_uart_write(&"HELLO", strlen("HELLO"));
  }
}

// echo to either Serial0 or Serial1
// with Serial0 as all lower case, Serial1 as all upper case
static void echo_serial_port(uint8_t itf, uint8_t buf[], uint32_t count)
{
  uint8_t const case_diff = 'a' - 'A';

  for (uint32_t i = 0; i < count; i++)
  {
    if (itf == 0)
    {
      // echo back 1st port as lower case
      if (isupper(buf[i]))
        buf[i] += case_diff;
    }
    else
    {
      // echo back 2nd port as upper case
      if (islower(buf[i]))
        buf[i] -= case_diff;
    }

    tud_cdc_n_write_char(itf, buf[i]);
  }
  tud_cdc_n_write_flush(itf);
}

// Invoked when device is mounted
void tud_mount_cb(void)
{
  blink_interval_ms = BLINK_MOUNTED;
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
  blink_interval_ms = BLINK_NOT_MOUNTED;
}

//--------------------------------------------------------------------+
// USB CDC
//--------------------------------------------------------------------+
static void cdc_task(void)
{
  uint8_t itf;

  for (itf = 0; itf < CFG_TUD_CDC; itf++)
  {
    // connected() check for DTR bit
    // Most but not all terminal client set this when making connection
    // if ( tud_cdc_n_connected(itf) )
    {
      if (tud_cdc_n_available(itf))
      {
        uint8_t buf[64];

        uint32_t count = tud_cdc_n_read(itf, buf, sizeof(buf));

        // echo back to both serial ports
        echo_serial_port(0, buf, count);
        // echo_serial_port(1, buf, count);
      }
    }
  }
}

// Invoked when cdc when line state changed e.g connected/disconnected
// Use to reset to DFU when disconnect with 1200 bps
void tud_cdc_line_state_cb(uint8_t instance, bool dtr, bool rts)
{
  (void)rts;

  // DTR = false is counted as disconnected
  if (!dtr)
  {
    // touch1200 only with first CDC instance (Serial)
    if (instance == 0)
    {
      cdc_line_coding_t coding;
      tud_cdc_get_line_coding(&coding);
      if (coding.bit_rate == 1200)
      {
        if (board_reset_to_bootloader)
        {
          board_reset_to_bootloader();
        }
      }
    }
  }
}

//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+
void led_blinking_task(void)
{
  static uint32_t start_ms = 0;
  static bool led_state = false;

  // Blink every interval ms
  if (board_millis() - start_ms < blink_interval_ms)
    return; // not enough time
  start_ms += blink_interval_ms;

  board_led_write(led_state);
  led_state = 1 - led_state; // toggle
}
