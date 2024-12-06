#include <Arduino.h>
#include <avr/sleep.h>
#include "Wire.h"
#include "BQ25180.hpp"
#include "mcuRegisterAlias.hpp"

#define colorPWMFactor 4

BQ25180 chargerIC;

uint8_t rTarget = 0;
uint8_t gTarget = 0;
uint8_t bTarget = 0;

void setup() {

  // oscillator calibration config
  CCP = CCP_IOREG_gc;
  FUSE.OSCCFG &= ~(1 << FUSE_OSCLOCK_bp); // oscillator unlocked, can make config changes
  CCP = CCP_IOREG_gc;
  CLKCTRL.OSC20MCALIBA = 0b0100100; // oscillator adjustment value, this value is likely to be different for each mcu

  Serial.begin(9600);
  R_LED_reg.DIR |= R_LED_bm; // red led set mode output
  G_LED_reg.DIR |= G_LED_bm; // green led set mode output
  B_LED_reg.DIR |= B_LED_bm; // blue led set mode output

  // CCP = CCP_IOREG_gc;
  // CLKCTRL.MCLKCTRLB |= 0x4 | CLKCTRL_PEN_bm;  // div 32 main clock prescaler, clock prescaler enabled

  TCA0.SINGLE.PER = 120;  // period, count to from zero before firing interrupt
  TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
  TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV64_gc | TCA_SINGLE_ENABLE_bm;  // enable counter with /64 counter prescaling

  set_sleep_mode(SLEEP_MODE_IDLE);

  if (chargerIC.begin()) {
    chargerIC.dumpInfo();
    gTarget = 20;
  } else {
    rTarget = 20;
  }

  // delay(1000);
  
  sei();
}

void loop() {
  // sleep_mode();
}

// count only used within the scope of TCA0_OVF_vect, volatile not needed
uint8_t count = 0;
ISR(TCA0_OVF_vect) {
  uint8_t rTargetAdjusted = rTarget / colorPWMFactor;
  uint8_t gTargetAdjusted = gTarget / colorPWMFactor;
  uint8_t bTargetAdjusted = bTarget / colorPWMFactor;

  if (count <= rTargetAdjusted && rTargetAdjusted > 0) {
    R_LED_reg.OUT |= R_LED_bm; // set output high
  } else {
    R_LED_reg.OUT &= ~R_LED_bm; // set output low
  }

  if (count <= gTargetAdjusted && gTargetAdjusted > 0) {
    G_LED_reg.OUT |= G_LED_bm; // set output high
  } else {
    G_LED_reg.OUT &= ~G_LED_bm; // set output low
  }

  if (count <= bTargetAdjusted && bTargetAdjusted > 0) {
    B_LED_reg.OUT |= B_LED_bm; // set output high
  } else {
    B_LED_reg.OUT &= ~B_LED_bm; // set output low
  }

  count += 1;

  if (count > 255 / colorPWMFactor) {
    count = 0;
  }

  TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
}


// life cycle

// first power after completely dead

// when off, on any click, if unplugged, check battery voltage, 
//   if battery voltage low, indicator color flashes and turn off.
// if locked and unlock pattern released at turn time, turn on, else shut off
// periodically sample battery voltage and shut down with battery indicator flashes.
// hold button menu while on, make these reorderable.
//   short hold -> off
//   medium hold -> change profile
//   long hold -> lock