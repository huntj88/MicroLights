#define INT0_INTERRUPT_ENABLE GIMSK // register to enable INT0 as interrupt vector
#define INT0_INTERRUPT_CONFIG MCUCR // register to configure INT0 interrupt to happen on low level
#define CLOCK_INTERRUPT_ENABLE TIMSK // register to enable Output Compare A Match clock interrupt


#define LedPin (1<<PB1)
#define ClockCountForInterrupt 2880 //2880
#define ClockInterruptRate 347        // 1 Mhz / 2880 = 347 hertz, default of 1Mhz (8Mhz clock speed with system clock prescale of 8)
#define AutoOffTimeSeconds (60 * 30)  // 30 minutes
#define AutoOffCounterPrescale 60
#define ClockInterruptsUntilAutoOff (AutoOffTimeSeconds / AutoOffCounterPrescale * ClockInterruptRate)
