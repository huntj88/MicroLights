void setup() {
  pinMode(5, OUTPUT);
  CCP = CCP_IOREG_gc;
  CLKCTRL.MCLKCTRLB |= 0x4 | CLKCTRL_PEN_bm; // div 32 main clock prescaler, clock prescaler enabled

  TCA0.SINGLE.PER = 0b1111111111111111; // max 16 bit uint value of 65,536
  TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
  TCA0.SINGLE.CTRLA = 1 | 0x7; // enable counter with /1024 counter prescaling

  sei();
}

void loop() {

}

bool on = true;
uint8_t count = 0;
ISR(TCA0_OVF_vect) {
  if (count < 14) {
    digitalWrite(5, HIGH);
  } else {
    digitalWrite(5, LOW);
    count = 0;
  }

  count += 1;
  TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
}