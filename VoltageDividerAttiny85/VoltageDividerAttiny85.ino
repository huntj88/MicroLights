#define LED_PIN (1 << PB3)
#define ADC_PIN (1 << PB4)

const uint8_t INPUT_ADC = 2;
const uint8_t ADC_PRESCALER = _BV(ADPS1) | _BV(ADPS0);  // Set prescaler to 8 for 1MHz CPU = 125KHz

const uint8_t N_BYTES = 2;
volatile uint8_t bytes[N_BYTES];

void setup() {

  ACSR |= (1 << ACD);  // disable Analog Comparator (power savings, module not needed, enabled by default)
  DDRB = (1 << DDB3);

  sei();
}

void loop() {


  takeAdcMeasurement();

  while (ADCSRA & (1 << ADSC))
    ;  //wait for conversion to end

  // flash to indicate loop progression
  PORTB |= LED_PIN;
  delay(5);
  PORTB &= ~LED_PIN;
  delay(5);
  PORTB |= LED_PIN;
  delay(5);
  PORTB &= ~LED_PIN;
  delay(5);

  // keep led on while voltage is above 3.1v (with the chosen resistors in my voltage divider)
  if (bytes[0] > (1 << 0)) {
    PORTB |= LED_PIN;
  } else {
    PORTB &= ~LED_PIN;
  }

  delay(100);
}

void takeAdcMeasurement() {
  PRR &= ~_BV(PRADC);  //power on ADC
  ADMUX = INPUT_ADC;   // Use 1.1V as reference, single-ended on input adc
  ADMUX |= (1 << REFS1);
  ADCSRA = _BV(ADEN) | _BV(ADSC) | _BV(ADIE) | ADC_PRESCALER;  // enable ADC & start conversion
}

// ADC interrupt vector
ISR(ADC_vect) {
  bytes[1] = ADCL;
  bytes[0] = ADCH;

  ADCSRA &= ~_BV(ADEN);
  ADCSRA &= ~_BV(ADSC);
  PRR |= _BV(PRADC);  // power off ADC
}