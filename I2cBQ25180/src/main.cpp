#include <Arduino.h>
#include "BQ25180.hpp"

BQ25180 chargerIC;

void setup() {
  Serial.begin(9600);
  while (!Serial); // wait for serial monitor
  if (chargerIC.begin()) {
    Serial.println("connected");
    chargerIC.dumpInfo();
  } else {
    Serial.println("error");
  }
}

void loop() {
  // put your main code here, to run repeatedly:
}
