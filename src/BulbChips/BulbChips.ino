#include <avr/sleep.h>

#define LedPin 0b0001  // pin 1
#define ClockCountForInterrupt 720
#define SystemClockRate 250000                                     // 250 kilohertz (8Mhz / 32 system clock prescale)
#define ClockInterruptRate SystemClockRate / ClockCountForInterrupt  // 347 hertz
#define AutoOffTimeSeconds 60 * 30                                   // 30 minutes
#define AutoOffCounterPrescale 60
#define ClockInterruptsUntilAutoOff AutoOffTimeSeconds / AutoOffCounterPrescale* ClockInterruptRate

volatile bool clickStarted = false;
volatile int mode = 0;
volatile int autoOffClockInterruptCount = 0;

// do nothing, run inputLoop instead.
void loop() {}

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

  inputLoop();
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
  CCP = 0xD8;                                                              // write signature to change clock settings
  CLKPSR = (0 << CLKPS3) | (1 << CLKPS2) | (0 << CLKPS1) | (1 << CLKPS0);  // system clock prescaler. system clock = main clock / 32
}

// clock is configured to fire the interrupt at a rate of 347 hertz
void setUpClockCounterInterrupt() {
  TCCR0A = 0;                                        // unset bits for clock
  OCR0A = ClockCountForInterrupt;                    // set Output Compare A value
  TCCR0B = (0 << CS02) | (0 << CS01) | (1 << CS00);  // no counter prescaling
  TCCR0B |= (1 << WGM02);                            // clear clock counter when counter reaches OCR0A value
  TIMSK0 |= (1 << OCIE0A);                           // enable Output Compare A Match clock interrupt, the interrupt is called every time the counter counts to the OCR0A value
}

void inputLoop() {
  int buttonDownCounter = 0;  // Used for detecting clicking button vs holding button by counting up or down to debounce button noise.
  while (true) {
    if (clickStarted) {
      // Disable interrupt (INT0) when button is clicked until:
      // * Interrupt is enabled prior to shutdown.
      // * The click has ended.
      EIMSK &= ~(1 << INT0);

      bool buttonCurrentlyDown = !(PINB & (1 << PB2));

      if (buttonCurrentlyDown) {
        buttonDownCounter += 1;
        if (buttonDownCounter > 200) {
          buttonDownCounter = 0;
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
    } else if (autoOffClockInterruptCount > ClockInterruptsUntilAutoOff) {
      shutdown(false);
    }

    // put CPU to into idle mode until clock interrupt sets led high/low state before releasing control back to main loop.
    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_mode();
  }
}

/**
 * Lock and unlock sequence.
 * flashes: blank -> mode 1 -> blank -> mode 2 -> off. If released on mode 2, the chip will be locked/unlocked.
 * If locked/unlocked successfully, it will flash mode 1 -> mode 2 in rapid succession and then shutdown/wakeup
 *
 * If released at any other point, a normal shutdown occurs.
 */
bool checkLockSequence() {
  int buttonDownCounter = 0;  // Used for detecting clicking button vs holding button by counting up or down to debounce button noise.

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

    if (buttonDownCounter > 800) {
      canceled = true;
      buttonDownCounter = 800;  // prevent long hold counter during locking sequence
      OCR0A = 5000;             // set Output Compare A value, increase the amount of time in idle while button held forever by user accident.
    } else {
      OCR0A = ClockCountForInterrupt;  // set Output Compare A value
    }

    bool showFirstBlank = buttonDownCounter > 0 && buttonDownCounter < 200;
    bool showSecondBlank = buttonDownCounter > 400 && buttonDownCounter < 600;

    if (canceled || (!completed && buttonDownCounter < 0) || showFirstBlank || showSecondBlank) {
      PORTB &= ~LedPin;  // Set GPIO1 to LOW
    }

    if (buttonDownCounter > 600) {
      // button held until mode 2 in lock sequence, release to lock, hold to cancel lock.
      completed = true;
      mode = 2;
    } else if (buttonDownCounter > 200) {
      mode = 1;
    }

    if (buttonDownCounter < 0 && completed && !canceled) {
      mode = 2;  // part of flashing mode 2 on success
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
  mode = -1;  // click started on wakeup from button interrupt, will increment to mode 0 in inputLoop.

  cli();             // disable interrupts
  PORTB &= ~LedPin;  // Set GPIO1 to LOW

  TIMSK0 &= ~(1 << OCIE0A);  // disable Output Compare A Match clock interrupt
  EIMSK |= (1 << INT0);      // Enable INT0 as interrupt vector
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sei();         // enable interrupts
  sleep_mode();  // sleep

  TIMSK0 |= (1 << OCIE0A);  // enable Output Compare A Match clock interrupt

  if (lock || wasLocked) {
    EIMSK &= ~(1 << INT0);
    bool unlock = checkLockSequence();
    mode = -1;  // click started on wakeup from button interrupt, will increment to mode 0 in inputLoop.
    if (!unlock) {
      shutdown(true);
    }
  }
}

ISR(INT0_vect) {
  clickStarted = true;
  autoOffClockInterruptCount = 0;
}

int clockInterruptCount = 0;  // only used in the scope TIM0_COMPA_vect to keep track of how many times the interrupt has happened.
int modeInterruptCount = 0;   // only used in the scope TIM0_COMPA_vect to track flashing progressiong for each mode.
ISR(TIM0_COMPA_vect) {
  clockInterruptCount += 1;
  modeInterruptCount += 1;

  if (clockInterruptCount % AutoOffCounterPrescale == 0) {
    // 16 bit ints can't count high enough, prescale
    autoOffClockInterruptCount += 1;
  }

  // Flashing patterns defined below.

  // Set led output to high every clock interrupt,
  // may be set to low to turn led off before high takes effect
  PORTB |= LedPin;  //  Set GPIO1 to HIGH

  switch (mode) {
    case 0:
      break;

    case 1:
      if (modeInterruptCount > 5) {
        PORTB &= ~LedPin;  //  Set GPIO1 to LOW
        modeInterruptCount = 0;
      }
      break;

    case 2:
      if (modeInterruptCount > 2) {
        PORTB &= ~LedPin;  //  Set GPIO1 to LOW
        modeInterruptCount = 0;
      }
      break;

    case 3:
      if (modeInterruptCount > 8) {
        PORTB &= ~LedPin;  //  Set GPIO1 to LOW
      }
      if (modeInterruptCount > 30) {
        modeInterruptCount = 0;
      }
      break;

    case 4:
      if (modeInterruptCount % 3 == 0 && modeInterruptCount < 80) {
        PORTB &= ~LedPin;  //  Set GPIO1 to LOW
      }
      if (modeInterruptCount > 100) {
        modeInterruptCount = 0;
      }
      break;
  }
}
