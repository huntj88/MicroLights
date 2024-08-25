#include <avr/sleep.h>

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

uint8_t count = 0;

uint8_t rTarget = 255;
uint8_t gTarget = 25;
uint8_t bTarget = 25;

ISR(TCA0_OVF_vect) {

  if (count < rTarget / 10) {
    digitalWrite(5, HIGH);
  } else {
    digitalWrite(5, LOW);
  }

  if (count < gTarget / 10) {
    digitalWrite(4, HIGH);
  } else {
    digitalWrite(4, LOW);
  }

  if (count < bTarget / 10) {
    digitalWrite(3, HIGH);
  } else {
    digitalWrite(3, LOW);
  }

  count += 1;

  if (count > 255 / 10) {
    count = 0;
  }

  TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
}