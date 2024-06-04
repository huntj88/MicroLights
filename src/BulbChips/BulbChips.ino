#include <util/delay.h>
#include <avr/sleep.h>

#define B1 0b0001

volatile bool clickStarted = false;
volatile bool on = true;

// TODO: use clock for consistent hold times, counter can be influenced by different mode delays
volatile int heldCounter = 0;

int mode = 0;

void setup() {
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  DDRB = 0b0001;  // set pin 1 as output
  PUEB = 0b1110;  // pullups on input pin 2, as well as 3,4 (unused)

  PCICR = 0b00000001;      // Pin Change Interrupt Control Register set to enable PCINT Interrupt
  PCMSK |= (1 << PCINT2);  // Select pins for interrupt, pb2 in this case.

  sei();  // Enable interrupts
}

void loop() {
  if (!on) {
    turnOff();
  }

  bool buttonCurrentlyDown = !(PINB & (1 << PB2));

  // checking if held until power off
  if (clickStarted && buttonCurrentlyDown) {
    heldCounter += 1;
    if (heldCounter > 50) {
      on = false;
      return;
    }
  }

  // click started and released, incrementing mode
  if (clickStarted && !buttonCurrentlyDown) {
    mode += 1;
    clickStarted = false;
    heldCounter = 0;
    if (mode > 2) {
      mode = 0;
    }
  }

  if (mode == 0) {
    // max power
    PORTB |= B1;  //  Set GPIO1 to HIGH
    _delay_ms(40);
  } else if (mode == 1) {
    PORTB &= ~B1;  //  Set GPIO1 to LOW
    _delay_ms(1);
    PORTB |= B1;  //  Set GPIO1 to HIGH
    _delay_ms(20);
  } else if (mode == 2) {
    PORTB &= ~B1;  //  Set GPIO1 to LOW
    _delay_ms(1);
    PORTB |= B1;  //  Set GPIO1 to HIGH
    _delay_ms(11);
  }
}

// invoked from pin change interupt (switch on PB2), when logic level changes
ISR(PCINT0_vect) {
  if (!on) {
    return;
  }

  bool buttonCurrentlyDown = !(PINB & (1 << PB2));

  if (heldCounter > 50 && !buttonCurrentlyDown) {
    // releasing button, can trigger wake, turn back off
    on = false;
  } else if (!clickStarted && buttonCurrentlyDown) {
    heldCounter = 0;
    clickStarted = true;
  }
}

void turnOff() {
  PORTB &= ~B1;  //  Set GPIO1 to LOW
  sleep_mode();

  // will start executing here when coming out of sleep, re-init.
  _delay_ms(100);  // prevent race case of interrupt from releasing button after powering off, and heldCounter being set to 0 then waking back up again due to the interrupt.
  mode = -1;
  clickStarted = false;
  heldCounter = 0;
  on = true;
}
