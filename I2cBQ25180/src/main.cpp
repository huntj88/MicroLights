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

void setup()
{
  Serial.begin(9600);
  while (!Serial)
    ; // wait for serial monitor
  if (chargerIC.begin())
  {
    chargerIC.dumpInfo();
    if (configureRegister_IC_CTRL())
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
