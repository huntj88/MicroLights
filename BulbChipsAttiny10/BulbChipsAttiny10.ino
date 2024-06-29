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
#include "RegisterAlias.h"

#define LedPin 0b0001  // pin 1 (PB0)
#define ClockCountForInterrupt 2880
#define ClockInterruptRate 347        // 1 Mhz / 2880 = 347 hertz, default of 1Mhz (8Mhz clock speed with system clock prescale of 8)
#define AutoOffTimeSeconds (60 * 30)  // 30 minutes
#define AutoOffCounterPrescale 60
#define ClockInterruptsUntilAutoOff (AutoOffTimeSeconds / AutoOffCounterPrescale * ClockInterruptRate)

volatile bool clickStarted = false;
volatile int clockInterruptCount = 0;
volatile int mode = 0;

void setup() {
  handleReset();  // user may have used the reset function, disable watch dog timer.

  DDRB = 0b0001;  // set pin 1 as output
  PUEB = 0b1110;  // pullups on input pin 2 (button), as well as 3,4 (unused)

  // Set interrupt on button press
  INT0_INTERRUPT_ENABLE |= (1 << INT0);                 // Enable INT0 as interrupt vector
  INT0_INTERRUPT_CONFIG = (0 << ISC01) | (0 << ISC00);  // Low level on INT0 generates an interrupt request

  setUpClockCounterInterrupt();

  ACSR |= (1 << ACD);  // disable Analog Comparator (power savings, module not needed, enabled by default)

  sei();  // Enable interrupts
}

// clock is configured to fire the interrupt at a rate of 347 hertz
void setUpClockCounterInterrupt() {
  OCR0A = ClockCountForInterrupt;                    // set Output Compare A value
  TCCR0B = (0 << CS02) | (0 << CS01) | (1 << CS00);  // no counter prescaling
  TCCR0B |= (1 << WGM02);                            // clear clock counter when counter reaches OCR0A value
  CLOCK_INTERRUPT_ENABLE |= (1 << OCIE0A);                           // enable Output Compare A Match clock interrupt, the interrupt is called every time the counter counts to the OCR0A value
}

void loop() {
  // investigating rare error, can potentially be removed, or if I never see the error again, I'll update this comment.
  setUpClockCounterInterrupt();

  // instead of invoking shutdown/lockSequence from inputLoop, separate them and get an empty stack to work with for each.
  inputLoop();
  bool lock = checkLockSequence();  // can trigger a reset
  shutdown(lock);
}

/**
 * Reset provided to make it easy for users to fix any rare issues the darn engineer missed.
 */
void resetChip() {
  CCP = 0xD8;
  WDTCSR = (1 << WDE);  // WDE: Watchdog System Reset Enable, watch dog timer with default config will reset 16 milliseconds after enabled.
  while (true) {}
}

void handleReset() {
  CCP = 0xD8;
  RSTFLR &= ~(1 << WDRF);  // clear bit, watch dog reset may have occurred, will bootloop until cleared, must happen before WDE is cleared.
  CCP = 0xD8;
  WDTCSR &= ~(1 << WDE);  // disable watchdog reset, will bootloop until cleared.
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
