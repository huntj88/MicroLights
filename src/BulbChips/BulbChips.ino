#include <avr/sleep.h>

#define B1 0b0001

volatile bool clickStarted = false;
volatile int mode = 0;

int buttonDownCounter = 0;  // Used for detecting clicking button vs holding button by counting up or down to debounce button noise.

void setup() {
  setSystemClockSpeed();

  DDRB = 0b0001;  // set pin 1 as output
  PUEB = 0b1110;  // pullups on input pin 2 (button), as well as 3,4 (unused)

  // Set interrupt on button press
  EIMSK |= (1 << INT0);                 // Enable INT0 as interrupt vector
  EICRA = (0 << ISC01) | (0 << ISC00);  // Low level on INT0 generates an interrupt request

  setUpClockCounterInterrupt();

  ACSR |= (1 << ACD);  // disable Analog Comparator (power savings, module not needed, enabled by default)

  sei();  // Enable interrupts
}

void setSystemClockSpeed() {
  /**
 * In order to change the contents of a protected I/O register the CCP register must first be written with the correct signature.
 * After CCP is written the protected I/O registers may be written to during the next four CPU instruction cycles. All interrupts
 * are ignored during these cycles. After these cycles interrupts are automatically handled again by the CPU, and any pending
 * interrupts will be executed according to their priority.
 */
  CCP = 0xD8;                                                              // write signature to change clock settings
  CLKMSR = 0;                                                              // using internal 8mhz oscillator for clock
  CLKPSR = (1 << CLKPS3) | (0 << CLKPS2) | (0 << CLKPS1) | (0 << CLKPS0);  // system clock prescaler. system clock = main clock / 256
}

void setUpClockCounterInterrupt() {
  TCCR0A = 0;                                        // unset bits for clock
  OCR0A = 50;                                        // set Output Compare A value
  TCCR0B = (0 << CS02) | (1 << CS01) | (1 << CS00);  // set up clock counter to increment every 64 system clock cycles (/64 prescaling)
  TCCR0B |= (1 << WGM02);                            // clear clock counter when counter reaches OCR0A value
  TIMSK0 |= (1 << OCIE0A);                           // enable Output Compare A Match clock interrupt, the interrupt is called every time the counter counts to the OCR0A value
}

void loop() {
  if (clickStarted) {
    // Disable interrupt (INT0) when button is clicked until:
    // * Interrupt is enabled prior to shutdown.
    // * The click has ended.
    EIMSK &= ~(1 << INT0);

    bool buttonCurrentlyDown = !(PINB & (1 << PB2));

    if (buttonCurrentlyDown) {
      buttonDownCounter += 1;
      if (buttonDownCounter > 200) {
        // Hold button down to shut down.
        shutdown(false);
      }
    } else {
      buttonDownCounter -= 10;  // Large decrement to allow any hold time to "discharge" quickly.
      if (buttonDownCounter < -200) {
        // Button clicked and released.
        clickStarted = false;
        buttonDownCounter = 0;
        mode += 1;
        if (mode > 4) {
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

/**
 * Lock and unlock sequence.
 * flashes: blank -> mode 1 -> blank -> mode 2. If released on mode 2, the chip will be locked/unlocked.
 * If locked, it will flash mode 1 -> mode 2 in rapid succession.
 *
 * If released at any other point, a normal shutdown occurs.
 */
bool checkLockSequence() {
  int buttonDownCounter = 0;

  bool completed = false;
  bool canceled = false;
  while (true) {
    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_mode();

    bool buttonCurrentlyDown = !(PINB & (1 << PB2));

    if (buttonCurrentlyDown) {
      buttonDownCounter += 1;
    } else {
      buttonDownCounter -= 5;
    }

    if (canceled || (!completed && buttonDownCounter < 0) || (buttonDownCounter > 0 && buttonDownCounter < 200) || (buttonDownCounter > 400 && buttonDownCounter < 600) || buttonDownCounter > 800) {
      PORTB &= ~B1;  // Set GPIO1 to LOW
    }

    if (buttonDownCounter > 600) {
      mode = 2;
    } else if (buttonDownCounter > 200) {
      mode = 1;
    }

    if (buttonDownCounter > 600) {
      // button held past shutdown sequence.
      completed = true;
    }

    if (buttonDownCounter > 800) {
      canceled = true;
    }

    if (buttonDownCounter > 800) {
      // prevent long hold counter during locking sequence
      buttonDownCounter = 800;

      // increase the amount of time in idle while button held forever by user accident.
      OCR0A = 5000;  // set Output Compare A value
    } else {
      OCR0A = 50;  // set Output Compare A value
    }

    if (buttonDownCounter < 0 && completed && !canceled) {
      mode = 2;  // user lets go during mode 2
    }

    if (buttonDownCounter < -200) {
      // button released after shutdown started or lock sequence, break out of loop, continue shutdown
      break;
    }
  }

  return completed && !canceled;
}

void shutdown(bool wasLocked) {
  clickStarted = false;

  bool lock = checkLockSequence();

  buttonDownCounter = 0;

  cli();         // disable interrupts
  PORTB &= ~B1;  // Set GPIO1 to LOW

  mode = -1;  // click started on wakeup from button interrupt, will increment to mode 0 in main loop.

  TIMSK0 &= ~(1 << OCIE0A);  // disable Output Compare A Match clock interrupt
  EIMSK |= (1 << INT0);      // Enable INT0 as interrupt vector
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sei();         // enable interrupts
  sleep_mode();  // sleep

  // clock can get weird after awhile?
  setSystemClockSpeed();

  TIMSK0 |= (1 << OCIE0A);  // enable Output Compare A Match clock interrupt

  if (lock || wasLocked) {
    EIMSK &= ~(1 << INT0);
    bool unlock = checkLockSequence();
    if (!unlock) {
      shutdown(true);
    }
  }
}

ISR(INT0_vect) {
  clickStarted = true;
}

int clockCount = 0;  // only used in the scope TIM0_COMPA_vect to track how many times we get a clock interrupt.
ISR(TIM0_COMPA_vect) {
  clockCount += 1;

  // Flashing patterns defined below.

  // Set led output to high every clock count,
  // may be set to low to turn led off before high takes effect
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

  if (mode == 4) {
    if (clockCount % 3 == 0 && clockCount < 80) {
      PORTB &= ~B1;  //  Set GPIO1 to LOW
    }

    if (clockCount > 100) {
      clockCount = 0;
    }
  }
}
