#include <util/delay.h>
#include <avr/sleep.h>

#define B1 0b0001

volatile bool clickStarted = false;

volatile int heldCounter = 0;

volatile int clockCount = 0;

volatile int mode = 0;

void setup() {
  CCP = 0xD8;
  CLKMSR = 0;                                                              // internal osc
  CLKPSR = (1 << CLKPS3) | (0 << CLKPS2) | (0 << CLKPS1) | (0 << CLKPS0);  //div 256
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  DDRB = 0b0001;  // set pin 1 as output
  PUEB = 0b1110;  // pullups on input pin 2, as well as 3,4 (unused)

  // Set interrupt on button press
  EIMSK |= (1 << INT0);                 // Enable INT0 as interrupt vector
  EICRA = (0 << ISC01) | (0 << ISC00);  // Low level on INT0 generates an interrupt request

  TCCR0A = 0;                                                       // unset bits for clock
  TCCR0B = (0 << CS02) | (1 << CS01) | (0 << CS00) | (1 << WGM02);  // set up clock to have /8 prescaling and to have Clear Timer on Compare
  OCR0A = 100;                                                      // set Output Compare A value, will count to that value, call the interrupt, then count will reset to 0
  TIMSK0 |= (1 << OCIE0A);                                          // enable Output Compare A Match interrupt

  sei();  // Enable interrupts
}

void loop() {
  __asm__("nop");  // let sync happen so pin can be read
  __asm__("nop");
  __asm__("nop");
  bool buttonCurrentlyDown = !(PINB & (1 << PB2));

  // checking if held until power off
  if (clickStarted) {
    cli();

    if (buttonCurrentlyDown) {
      heldCounter += 1;
      if (heldCounter > 20000) {
        shutdown();
      }
    } else {
      heldCounter -= 1;
      if (heldCounter < -2000) {
        mode += 1;
        clickStarted = false;
        heldCounter = 0;
        if (mode > 2) {
          mode = 0;
        }
        sei();
      }
    }
  }

  PORTB |= B1;  //  Set GPIO1 to HIGH
}

ISR(INT0_vect) {
  clickStarted = true;
}

ISR(TIM0_COMPA_vect) {
  clockCount += 1;
  if (mode == 1 && clockCount > 20) {
    PORTB &= ~B1;  //  Set GPIO1 to LOW
    clockCount = 0;
    _delay_ms(1);  // delay long enough for led to register low level and reset
  }

  if (mode == 2 && clockCount > 10) {
    PORTB &= ~B1;  //  Set GPIO1 to LOW
    clockCount = 0;
    _delay_ms(1);  // delay long enough for led to register low level and reset
  }
}

void shutdown() {
  cli();            // disable interrupts
  PORTB &= ~B1;     //  Set GPIO1 to LOW
  clickStarted = false;
  heldCounter = 0;
  mode = 0;
  _delay_ms(2000);  // user lets go during this interval
  sei();            // enable interrupts
  sleep_mode();     // sleep
}