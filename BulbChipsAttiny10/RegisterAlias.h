#define INT0_INTERRUPT_ENABLE EIMSK // register to enable INT0 as interrupt vector
#define INT0_INTERRUPT_CONFIG EICRA // register to configure INT0 interrupt to happen on low level
#define CLOCK_INTERRUPT_ENABLE TIMSK0 // register to enable Output Compare A Match clock interrupt


#define LedPin 0b0001  // pin 1 (PB0)
#define ClockCountForInterrupt 2880
#define ClockInterruptRate 347        // 1 Mhz / 2880 = 347 hertz, default of 1Mhz (8Mhz clock speed with system clock prescale of 8)
#define AutoOffTimeSeconds (60 * 30)  // 30 minutes
#define AutoOffCounterPrescale 60
#define ClockInterruptsUntilAutoOff (AutoOffTimeSeconds / AutoOffCounterPrescale * ClockInterruptRate)