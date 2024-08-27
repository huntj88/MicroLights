#include <Arduino.h>
#include <avr/sleep.h>

#define colorPWMFactor 8

uint8_t rTarget = 8;
uint8_t gTarget = 8;
uint8_t bTarget = 120;

void setup() {
  PORTC.DIR |= (1 << 0); // red led set mode output
  PORTC.DIR |= (1 << 1); // green led set mode output
  PORTC.DIR |= (1 << 2); // blue led set mode output

  // CCP = CCP_IOREG_gc;
  // CLKCTRL.MCLKCTRLB |= 0x4 | CLKCTRL_PEN_bm;  // div 32 main clock prescaler, clock prescaler enabled

  TCA0.SINGLE.PER = 2000;  // period, count to from zero before firing interrupt
  TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
  TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV64_gc | TCA_SINGLE_ENABLE_bm;  // enable counter with /64 counter prescaling

  set_sleep_mode(SLEEP_MODE_IDLE);

  sei();
}

void loop() {
  sleep_mode();
}

// count only used within the scope of TCA0_OVF_vect, volatile not needed
uint8_t count = 0;
ISR(TCA0_OVF_vect) {
  uint8_t rTargetAdjusted = rTarget / colorPWMFactor;
  uint8_t gTargetAdjusted = gTarget / colorPWMFactor;
  uint8_t bTargetAdjusted = bTarget / colorPWMFactor;

  if (count <= rTargetAdjusted && rTargetAdjusted > 0) {
    PORTC.OUT |= (1 << 0); // green led set output high
  } else {
    PORTC.OUT &= ~(1 << 0); // green led set output high
  }

  if (count <= gTargetAdjusted && gTargetAdjusted > 0) {
    PORTC.OUT |= (1 << 1); // green led set output high
  } else {
    PORTC.OUT &= ~(1 << 1); // green led set output high
  }

  if (count <= bTargetAdjusted && bTargetAdjusted > 0) {
    PORTC.OUT |= (1 << 2); // green led set output high
  } else {
    PORTC.OUT &= ~(1 << 2); // green led set output high
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