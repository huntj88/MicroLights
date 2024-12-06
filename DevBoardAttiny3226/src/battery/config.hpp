#include <Arduino.h>
#include "BQ25180.hpp"

bool configureRegister_IC_CTRL(BQ25180 *chargerIC)
{
  uint8_t newConfig = IC_CTRL_DEFAULT;
  uint8_t tsEnabledMask = 0b10000000;
  newConfig &= ~tsEnabledMask; // disable ts current changes (no thermistor on my project?)
  return chargerIC->write(BQ25180_IC_CTRL, newConfig);
}

// TODO: change to 70 milliamps?
bool configureRegister_ICHG_CTRL(BQ25180 *chargerIC)
{
  // enable charging = bit 7 in data sheet 0
  // 50 milliamp max charge current
  return chargerIC->write(BQ25180_ICHG_CTRL, 0b00100000);
}

// TODO: change to 4.4v?
bool configureRegister_VBAT_CTRL(BQ25180 *chargerIC)
{
  // 4.3v, (3.5v) + (80 * 10mV), 80 = 0b1010000
  return chargerIC->write(BQ25180_VBAT_CTRL, 0b01010000);
}

bool configureRegister_CHARGECTRL1(BQ25180 *chargerIC)
{
  // Battery Discharge Current Limit
  // 2b00 = 500mA

  // Battery Undervoltage LockOut Falling Threshold.
  // 3b000 = 3.0V

  // Mask Charging Status Interrupt = OFF 1b1
  // Mask ILIM Fault Interrupt = OFF 1b1
  // Mask VINDPM and VDPPM Interrupt = OFF 1b1, TODO: turn back on?, 1b0

  return chargerIC->write(BQ25180_CHARGECTRL1, 0b00000111);
}

// TODO: configure for more than just button presses.
bool configureRegister_MASK_ID(BQ25180 *chargerIC)
{
  // TS_INT_MASK: Mask or enable the TS interrupt.
  //   1b0: Enable TS interrupt
  //   1b1: Mask TS interrupt

  // TREG_INT_MASK: Mask or enable the TREG interrupt.
  //   1b0: Enable TREG interrupt
  //   1b1: Mask TREG interrupt

  // BAT_INT_MASK: Mask or enable BATOCP and BUVLO interrupts.
  //   1b0: Enable BATOCP and BUVLO interrupts
  //   1b1: Mask BATOCP and BUVLO interrupts

  // PG_INT_MASK: Mask or enable PG and VINOVP interrupts.
  //   1b0: Enable PG and VINOVP interrupts
  //   1b1: Mask PG and VINOVP interrupts

  // Device_ID: A 4-bit field indicating the device ID.
  //   4b0000: Device ID for the BQ25180.

  return chargerIC->write(BQ25180_MASK_ID, 0b01110000);
}

// TODO: SYS_REG_CTRL, SYS Regulation Voltage = Battery Tracking Mode
// TODO: interrupt masks

void setupBatteryCharger(BQ25180 *chargerIC)
{
  if (chargerIC->begin())
  {
    if (configureRegister_IC_CTRL(chargerIC) && configureRegister_ICHG_CTRL(chargerIC) && configureRegister_VBAT_CTRL(chargerIC) && configureRegister_CHARGECTRL1(chargerIC) && configureRegister_MASK_ID(chargerIC))
    {
      chargerIC->dumpInfo();
    }
    else
    {
      Serial.println("write error");
    }
  }
  else
  {
    Serial.println("connection error");
  }
}
