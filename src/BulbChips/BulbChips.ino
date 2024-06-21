#include <util/delay.h>
#include <avr/sleep.h>

#define B1 0b0001

volatile bool clickStarted = false;
volatile int mode = 0;

int heldCounter = 0;

void setup() {
  CCP = 0xD8;                                                              // set up ccp to change clock settings
  CLKMSR = 0;                                                              // using internal 8mhz oscillator for clock
  CLKPSR = (1 << CLKPS3) | (0 << CLKPS2) | (0 << CLKPS1) | (0 << CLKPS0);  // clock divided by 256

  DDRB = 0b0001;  // set pin 1 as output
  PUEB = 0b1110;  // pullups on input pin 2 (button), as well as 3,4 (unused)

  // Set interrupt on button press
  EIMSK |= (1 << INT0);                 // Enable INT0 as interrupt vector
  EICRA = (0 << ISC01) | (0 << ISC00);  // Low level on INT0 generates an interrupt request

  TCCR0A = 0;                                                       // unset bits for clock
  TCCR0B = (0 << CS02) | (1 << CS01) | (1 << CS00) | (1 << WGM02);  // set up clock to have /64 prescaling and to have Clear Timer on Compare
  OCR0A = 50;                                                       // set Output Compare A value, will count to that value, call the interrupt, then count will reset to 0
  TIMSK0 |= (1 << OCIE0A);                                          // enable Output Compare A Match clock interrupt

  sei();  // Enable interrupts
}

void loop() {
  if (clickStarted) {
    // Disable interrupts (INT0) when button is clicked until:
    // * Interrupt is enabled prior to shutdown.
    // * The click has ended.
    EIMSK &= ~(1 << INT0);

    bool buttonCurrentlyDown = !(PINB & (1 << PB2));

    if (buttonCurrentlyDown) {
      heldCounter += 1;
      if (heldCounter > 200) {
        shutdown();
      }
    } else {
      heldCounter -= 10;  // large decrement to allow any hold time to "discharge" quickly.
      if (heldCounter < -200) {
        mode += 1;
        clickStarted = false;
        heldCounter = 0;
        if (mode > 3) {
          mode = 0;
        }
        EIMSK |= (1 << INT0);  // Enable INT0 as interrupt vector
      }
    }
  }

  // put CPU to into idle mode until clock interrupt sets led high/low state before releasing control back to main loop.
  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_mode();
}

void shutdown() {
  cli();         // disable interrupts
  PORTB &= ~B1;  // Set GPIO1 to LOW
  clickStarted = false;
  heldCounter = 0;
  mode = -1;                 // click started on wakeup from button intterupt, will increment to mode 0 in main loop.
  
  _delay_ms(1000);           // user lets go during this interval

  TIMSK0 &= ~(1 << OCIE0A);  // disable Output Compare A Match clock interrupt
  EIMSK |= (1 << INT0);      // Enable INT0 as interrupt vector
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sei();                    // enable interrupts
  sleep_mode();             // sleep
  TIMSK0 |= (1 << OCIE0A);  // enable Output Compare A Match clock interrupt
}

ISR(INT0_vect) {
  clickStarted = true;
}

int clockCount = 0;  // only used in the scope TIM0_COMPA_vect to track how many times we get a clock interrupt.
ISR(TIM0_COMPA_vect) {
  clockCount += 1;

  // set led output to high every clock count, may be set to low to turn led off before high takes effect
  PORTB |= B1;  //  Set GPIO1 to HIGH

  if (mode == 1 && clockCount > 5) {
    PORTB &= ~B1;  //  Set GPIO1 to LOW
    clockCount = 0;
  }

  if (mode == 2 && clockCount > 2) {
    PORTB &= ~B1;  //  Set GPIO1 to LOW
    clockCount = 0;
  }

  if (mode == 3) {
    if (clockCount > 8) {
      PORTB &= ~B1;  //  Set GPIO1 to LOW
    }

    if (clockCount > 30) {
      clockCount = 0;
    }
  }
}
