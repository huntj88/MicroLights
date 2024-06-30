struct InputLoopParams {
  int (*getMode)();
  void (*setMode)(int);
  bool (*hasClickStarted)();
  void (*setClickEnded)();
  int (*getClockInterruptCount)();
};

// when this method returns, user is finished, and chip is now shutting down.
void inputLoop(InputLoopParams params) {
  int buttonDownCounter = 0;  // Used for detecting clicking button vs holding button by counting up or down to debounce button noise.
  int autoOffCounter = 0;     // 16 bit ints can't count high enough, clockInterruptCount prescaled by 60.
  while (true) {

    // input loop wakes up from sleep when clock interrupt fires
    // 16 bit ints can't count high enough, prescale by 60
    bool incrementAutoOff = params.getClockInterruptCount() % AutoOffCounterPrescale == 0;
    if (incrementAutoOff) {
      autoOffCounter += 1;
    }

    if (params.hasClickStarted()) {
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
          params.setClickEnded();
          buttonDownCounter = 0;
          int newMode = params.getMode();
          newMode += 1;
          if (newMode > 4) {
            newMode = 0;
          }
          params.setMode(newMode);
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
