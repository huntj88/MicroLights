#include <Arduino.h>
#include "BQ25180.hpp"
#include "setBit.h"

BQ25180 chargerIC;

bool configureRegister_IC_CTRL()
{
  uint8_t newConfig = IC_CTRL_DEFAULT;
  uint8_t tsEnabledMask = 0b10000000;
  setBit(&newConfig, tsEnabledMask, 0); // disable ts current changes (no thermistor on my project?)
  return chargerIC.write(BQ25180_IC_CTRL, newConfig);
}

// TODO: change to 70 milliamps?
bool configureRegister_ICHG_CTRL()
{
  // enable charging = bit 7 in data sheet 0
  // 50 milliamp max charge current
  return chargerIC.write(BQ25180_ICHG_CTRL, 0b00100000);
}

// TODO: change to 4.4v?
bool configureRegister_VBAT_CTRL()
{
  // 4.3v, (3.5v) + (80 * 10mV), 80 = 0b1010000
  return chargerIC.write(BQ25180_VBAT_CTRL, 0b01010000);
}

// TODO: SYS_REG_CTRL, SYS Regulation Voltage = Battery Tracking Mode
// bool configureRegister_VBAT_CTRL() {}

void setup()
{
  Serial.begin(9600);
  while (!Serial)
    ; // wait for serial monitor
  if (chargerIC.begin())
  {
    // chargerIC.dumpInfo();
    if (configureRegister_IC_CTRL() && configureRegister_ICHG_CTRL() && configureRegister_VBAT_CTRL())
    {
      chargerIC.dumpInfo();
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

void loop()
{
  // put your main code here, to run repeatedly:
}
