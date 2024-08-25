#include <avr/sleep.h>

#define colorPWMFactor 8

void setup() {
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  // CCP = CCP_IOREG_gc;
  // CLKCTRL.MCLKCTRLB |= 0x4 | CLKCTRL_PEN_bm;  // div 32 main clock prescaler, clock prescaler enabled

  TCA0.SINGLE.PER = 120;  // period, count to from zero before firing interrupt
  TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
  TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV64_gc | TCA_SINGLE_ENABLE_bm;  // enable counter with /64 counter prescaling

  set_sleep_mode(SLEEP_MODE_IDLE);

  sei();
}

void loop() {
  sleep_mode();
}

// only used within the scope of TCA0_OVF_vect, volatile potentially needed later when changing led color outside the scope of interrupt?
uint8_t rTarget = 255;
uint8_t gTarget = 19;
uint8_t bTarget = 19;

// only used within the scope of TCA0_OVF_vect, volatile not needed
uint8_t count = 0;
ISR(TCA0_OVF_vect) {
  uint8_t rTargetAdjusted = rTarget / colorPWMFactor;
  uint8_t gTargetAdjusted = gTarget / colorPWMFactor;
  uint8_t bTargetAdjusted = bTarget / colorPWMFactor;

  if (count <= rTargetAdjusted && rTargetAdjusted > 0) {
    digitalWrite(5, HIGH);
  } else {
    digitalWrite(5, LOW);
  }

  if (count <= gTargetAdjusted && gTargetAdjusted > 0) {
    digitalWrite(4, HIGH);
  } else {
    digitalWrite(4, LOW);
  }

  if (count <= bTargetAdjusted && bTargetAdjusted > 0) {
    digitalWrite(3, HIGH);
  } else {
    digitalWrite(3, LOW);
  }

  count += 1;

  if (count > 255 / colorPWMFactor) {
    count = 0;
  }

  TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
}