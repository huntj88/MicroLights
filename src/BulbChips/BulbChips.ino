#include <util/delay.h>
#include <avr/sleep.h>

#define B1 0b0001

volatile bool clicked = false;
int mode = 0;

void setup() {
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  DDRB = 0b0001;  // set pin 1 as output
  PUEB = 0b1110;  // pullups on input pin 2, as well as 3,4 (unused)

  // # Set interrupt on button press
  EIMSK |= (1 << INT0);   // Enable INT0 as interrupt vector
  // Falling edge of INT0 generates an interrupt request
  EICRA = (1 << ISC01) | (0 << ISC00);
  sei(); // Enable interrupts
}

int heldCounter = 0;

void loop() {
  bool stillPressed = !(PINB & (1<<PB2));
  if (clicked && stillPressed) {
    heldCounter += 1;
    if (heldCounter > 50) {
      turnOff();
    }
  }
  
  if (clicked && !stillPressed) {
    mode += 1;
    clicked = false;
    heldCounter = 0;
    if (mode > 2) {
      mode = 0;
    }
  }
  if (mode == 0) {
    // max power
    PORTB |= B1;    //  Set GPIO1 to HIGH
    _delay_ms(40);  
  } else if (mode == 1) {
    PORTB &= ~B1;   //  Set GPIO1 to LOW
    _delay_ms(1);   
    PORTB |= B1;    //  Set GPIO1 to HIGH
    _delay_ms(20);  
  } else if (mode == 2) {
    PORTB &= ~B1;   //  Set GPIO1 to LOW
    _delay_ms(1);   
    PORTB |= B1;    //  Set GPIO1 to HIGH
    _delay_ms(11);  
  }
}

ISR(INT0_vect) {
  clicked = true;
}

void turnOff() {
  mode = -1;
  clicked = false;
  heldCounter = 0;
  PORTB &= ~B1;   //  Set GPIO1 to LOW
  sleep_mode();
}