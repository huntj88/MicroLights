#include "RegisterAlias.h"

void shutdown(bool lock, int (*checkHoldSequence)()) {
  cli();             // disable interrupts
  PORTB &= ~LedPin;  // Set GPIO1 to LOW

  CLOCK_INTERRUPT_ENABLE &= ~(1 << OCIE0A);  // disable Output Compare A Match clock interrupt
  INT0_INTERRUPT_ENABLE |= (1 << INT0);      // Enable INT0 as interrupt vector
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sei();         // enable interrupts
  sleep_mode();  // sleep

  CLOCK_INTERRUPT_ENABLE |= (1 << OCIE0A);  // enable Output Compare A Match clock interrupt

  if (lock) {
    INT0_INTERRUPT_ENABLE &= ~(1 << INT0);
    bool unlock = checkHoldSequence();  // can trigger a reset
    if (unlock) {
      INT0_INTERRUPT_ENABLE |= (1 << INT0);  // Enable INT0 as interrupt vector
    } else {
      // Recursive shutdown call must be the last call to take advantage of tail recursion optimization.
      // Without tail recursion, repeated failures of unlocking the chips would result in a stack overflow.
      shutdown(true, checkHoldSequence);
    }
  }
}
