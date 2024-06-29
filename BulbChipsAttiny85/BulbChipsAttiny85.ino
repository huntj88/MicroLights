/**
Features
- 5 modes
  - Long (default led pattern)
  - Medium
  - Short
  - Blink Long then blank
  - short several times then long

- Battery Savings
  - Lock mode, see checkLockSequence() for details
  - Auto off after 30 minutes
  - If button held down accidentally, spend most of the time in idle mode

- Reset function if something weird is going on.
  - See checkLockSequence() for details
*/

#include <avr/sleep.h>
#include "common.h"

#define LedPin (1<<PB1)
#define ClockCountForInterrupt 2880 //2880
#define ClockInterruptRate 347        // 1 Mhz / 2880 = 347 hertz, default of 1Mhz (8Mhz clock speed with system clock prescale of 8)
#define AutoOffTimeSeconds (60 * 30)  // 30 minutes
#define AutoOffCounterPrescale 60
#define ClockInterruptsUntilAutoOff (AutoOffTimeSeconds / AutoOffCounterPrescale * ClockInterruptRate)

volatile bool clickStarted = false;
volatile int clockInterruptCount = 0;
volatile int mode = 4;

void setup() {
  cli();
  handleReset();  // user may have used the reset function, disable watch dog timer.

  DDRB |= (1<<DDB1);  // set PB1 as output
  PORTB |= 1 << PB2; // push button, when pin is input, setting PORTB high enables the pullup resister

  // Set interrupt on button press
  INT0_INTERRUPT_ENABLE |= (1 << INT0);                 // Enable INT0 as interrupt vector
  INT0_INTERRUPT_CONFIG |= (0 << ISC01) | (0 << ISC00);  // Low level on INT0 generates an interrupt request

  setUpClockCounterInterrupt();

  ACSR |= (1 << ACD);  // disable Analog Comparator (power savings, module not needed, enabled by default)

  sei();  // Enable interrupts
}

// clock is configured to fire the interrupt at a rate of 347 hertz
void setUpClockCounterInterrupt() {
  TCCR0A = 0;
  TCCR0B = 0;
  OCR0A = ClockCountForInterrupt;                    // set Output Compare A value
  TCCR0B |= (0 << CS02) | (1 << CS01) | (1 << CS00);  // /64 prescaling
  TCCR0A |= (1 << WGM01);// clear clock counter when counter reaches OCR0A value
  CLOCK_INTERRUPT_ENABLE |= (1 << OCIE0A);                           // enable Output Compare A Match clock interrupt, the interrupt is called every time the counter counts to the OCR0A value
}

void loop() {
  // // investigating rare error, can potentially be removed, or if I never see the error again, I'll update this comment.
  // setUpClockCounterInterrupt();

  // instead of invoking shutdown/lockSequence from inputLoop, separate them and get an empty stack to work with for each.
  
  inputLoop();
  bool lock = checkLockSequence();  // can trigger a reset
  shutdown(lock);
}

// when this method returns, user is finished, and chip is now shutting down.
void inputLoop() {
  int buttonDownCounter = 0;  // Used for detecting clicking button vs holding button by counting up or down to debounce button noise.
  int autoOffCounter = 0;     // 16 bit ints can't count high enough, clockInterruptCount prescaled by 60.
  while (true) {

    // input loop wakes up from sleep when clock interrupt fires
    // 16 bit ints can't count high enough, prescale by 60
    bool incrementAutoOff = clockInterruptCount % AutoOffCounterPrescale == 0;
    if (incrementAutoOff) {
      autoOffCounter += 1;
    }

    if (clickStarted) {
      autoOffCounter = 0;
      // Disable interrupt (INT0) when button is clicked until:
      // * Interrupt is enabled prior to shutdown.
      // * The click has ended.
      INT0_INTERRUPT_ENABLE &= ~(1 << INT0);

      bool buttonCurrentlyDown = !(PINB & (1 << PB2));

      if (buttonCurrentlyDown) {
        buttonDownCounter += 1;
        if (buttonDownCounter > 200) {
          // Hold button down to shut down.
          return;
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
          INT0_INTERRUPT_ENABLE |= (1 << INT0);  // Enable INT0 as interrupt vector
        }
      }
    } else if (autoOffCounter > ClockInterruptsUntilAutoOff) {
      // no user input auto off time elapsed, shutting down.
      return;
    }

    // put CPU to into idle mode until clock interrupt sets led high/low state before releasing control back to main loop.
    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_mode();
  }
}

void shutdown(bool lock) {
  cli();             // disable interrupts
  mode = -1;                        // click started on wakeup from button interrupt, will increment to mode 0 in inputLoop.
  PORTB &= ~LedPin;  // Set GPIO1 to LOW

  CLOCK_INTERRUPT_ENABLE &= ~(1 << OCIE0A);  // disable Output Compare A Match clock interrupt
  INT0_INTERRUPT_ENABLE |= (1 << INT0);      // Enable INT0 as interrupt vector
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sei();         // enable interrupts
  sleep_mode();  // sleep

  CLOCK_INTERRUPT_ENABLE |= (1 << OCIE0A);  // enable Output Compare A Match clock interrupt

  if (lock) {
    INT0_INTERRUPT_ENABLE &= ~(1 << INT0);
    bool unlock = checkLockSequence();  // can trigger a reset
    if (unlock) {
      mode = -1;             // click started on wakeup from button interrupt, will increment to mode 0 in inputLoop.
      INT0_INTERRUPT_ENABLE |= (1 << INT0);  // Enable INT0 as interrupt vector
    } else {
      // Recursive shutdown call must be the last call to take advantage of tail recursion optimization.
      // Without tail recursion, repeated failures of unlocking the chips would result in a stack overflow.
      shutdown(true);
    }
  }
}

/**
 * Lock and unlock sequence.
 * flashes: long blank -> mode 2 -> blank -> mode 0.
 * If released on mode 2, the chip will be locked/unlocked.
 * If released on mode 0, the chip with reset. Useful for solving any unknown issues that could happen.
 *
 * If locked/unlocked successfully, it will flash mode 2 once, then shutdown/wakeup.
 * If reset successfully, it will flash mode 0 -> mode 2 and then the microcontroller will reset.
 *
 * If released at any other point, a normal shutdown occurs.
 */
bool checkLockSequence() {
  int buttonDownCounter = 0;  // Used for detecting clicking button vs holding button by counting up or down to debounce button noise.

  bool completed = false;
  bool canceled = false;

  bool reset = false;
  bool resetCanceled = false;

  while (true) {
    // put CPU to into idle mode until clock interrupt
    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_mode();
    // clock interrupt woke up, has just set led high/low,
    // has now released control back to lock sequence loop,
    // can override led level to set blanks in lock sequence pattern.

    bool buttonCurrentlyDown = !(PINB & (1 << PB2));

    if (buttonCurrentlyDown) {
      buttonDownCounter += 1;
    } else {
      buttonDownCounter -= 5;
    }

    if (buttonDownCounter > 1200) {
      resetCanceled = true;
      buttonDownCounter = 1200;  // prevent long hold counter during locking sequence
      OCR0A = 65535;             // set Output Compare A value, increase the amount of time in idle while button held forever by user accident.
    } else {
      OCR0A = ClockCountForInterrupt;  // set Output Compare A value
    }

    if (buttonDownCounter > 800) {
      canceled = true;
    }

    bool blankBeforeLock = buttonDownCounter >= 0 && buttonDownCounter < 600;

    if ((canceled && !reset) || resetCanceled || (!completed && buttonDownCounter < 0) || blankBeforeLock) {
      PORTB &= ~LedPin;  // Set GPIO1 to LOW
    }

    if (buttonDownCounter >= 1000) {
      reset = true;
      mode = 0;
    } else if (buttonDownCounter >= 600) {
      // Button held until mode 2 in lock sequence, release to lock, hold to cancel lock.
      completed = true;
      mode = 2;
    }

    if (buttonDownCounter < 0 && completed && !canceled) {
      mode = 2;  // part of flashing mode 2 on success
    }

    if (buttonDownCounter < -200) {
      // button released after shutdown started or lock sequence, break out of loop, continue shutdown.
      break;
    }
  }

  if (reset && !resetCanceled) {
    // kind of a side effect of this function, but the whole chip resets anyway ¯\_(ツ)_/¯
    // might as well not expose the reset logic outside this function
    resetChip();
  }

  return completed && !canceled;
}

/**
 * Reset provided to make it easy for users to fix any rare issues the darn engineer missed.
 */
 // TODO
void resetChip() {
  // CCP = 0xD8;
  // WDTCSR = (1 << WDE);  // WDE: Watchdog System Reset Enable, watch dog timer with default config will reset 16 milliseconds after enabled.
  // while (true) {}
}

// TODO
void handleReset() {
  // CCP = 0xD8;
  // RSTFLR &= ~(1 << WDRF);  // clear bit, watch dog reset may have occurred, will bootloop until cleared, must happen before WDE is cleared.
  // CCP = 0xD8;
  // WDTCSR &= ~(1 << WDE);  // disable watchdog reset, will bootloop until cleared.
}

ISR(INT0_vect) {
  clickStarted = true;
}

int modeInterruptCount = 0;  // only used in the scope TIM0_COMPA_vect to track flashing progression for each mode.
ISR(TIM0_COMPA_vect) {
  clockInterruptCount += 1;
  modeInterruptCount += 1;

  // Flashing patterns defined below.

  // Set led output to high every clock interrupt,
  // may be set to low to turn led off before high takes effect
  PORTB |= 1 << PB1;

  switch (mode) {
    case 0:
      break;

    case 1:
      if (modeInterruptCount > 5) {
        PORTB &= ~LedPin;  //  Set to LOW
        modeInterruptCount = 0;
      }
      break;

    case 2:
      if (modeInterruptCount > 2) {
        PORTB &= ~LedPin;  //  Set to LOW
        modeInterruptCount = 0;
      }
      break;

    case 3:
      if (modeInterruptCount > 8) {
        PORTB &= ~LedPin;  //  Set to LOW
      }
      if (modeInterruptCount > 30) {
        modeInterruptCount = 0;
      }
      break;

    case 4:
      if (modeInterruptCount % 3 == 0 && modeInterruptCount < 80) {
        PORTB &= ~LedPin;  //  Set to LOW
      }
      if (modeInterruptCount > 100) {
        modeInterruptCount = 0;
      }
      break;
  }
}
