#include <util/delay.h>
#include <avr/sleep.h>

#define B1 0b0001

volatile bool clickStarted = false;

// TODO: use clock for consistent hold times, counter can be influenced by different mode delays
volatile int heldCounter = 0;

int mode = 0;

void setup() {
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  DDRB = 0b0001;  // set pin 1 as output
  PUEB = 0b1110;  // pullups on input pin 2, as well as 3,4 (unused)

  // Set interrupt on button press
  EIMSK |= (1 << INT0);                 // Enable INT0 as interrupt vector
  EICRA = (0 << ISC01) | (0 << ISC00);  // Low level on INT0 generates an interrupt request

  TCCR0A = 0; // unset bits for clock
  TCCR0B = (1 << CS00) | (1 << WGM02); // set up clock to have no prescaling and to have Clear Timer on Compare
  OCR0A = 50000; // set Output Compare A value, will count to that value, call the interrupt, then count will reset to 0
  TIMSK0 |= (1 << OCIE0A); // enable Output Compare A Match interrupt

  sei();  // Enable interrupts
}

void loop() {
  __asm__("nop");  // let sync happen so pin can be read
  __asm__("nop");
  __asm__("nop");
  bool buttonCurrentlyDown = !(PINB & (1 << PB2));

  // checking if held until power off
  if (clickStarted && buttonCurrentlyDown) {
    heldCounter += 1;
    if (heldCounter > 600) {
      shutdown();
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

    if (mode == 0) {
      OCR0A = 50000;  //todo: turn clock off full power?
    } else if (mode == 1) {
      OCR0A = 16000;
    } else if (mode == 2) {
      OCR0A = 7000;
    }

    sei();  // enable interrupts
  }

  PORTB |= B1;  //  Set GPIO1 to HIGH
}

ISR(INT0_vect) {
  cli();

  __asm__("nop");  // let sync happen so pin can be read
  __asm__("nop");
  __asm__("nop");
  bool buttonCurrentlyDown = !(PINB & (1 << PB2));

  clickStarted = buttonCurrentlyDown;
  if (!clickStarted) {
    sei();
  }
}

ISR(TIM0_COMPA_vect) {
  PORTB &= ~B1;  //  Set GPIO1 to LOW
  _delay_ms(1); // delay long enough for led to register low level and reset
}

void shutdown() {
  TIMSK0 &= ~(1 << OCIE0A);  // disable Output Compare A Match interrupt

  PORTB &= ~B1;    //  Set GPIO1 to LOW
  cli();           // disable interrupts
  _delay_ms(600);  // user lets go during this interval
  sei();           // enable interrupts
  sleep_mode();    // sleep

  // reset after sleep finished
  clickStarted = false;
  heldCounter = 0;
  mode = 0;


  TIMSK0 |= (1 << OCIE0A);  // enable Output Compare A Match interrupt
}