#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include <Arduino.h>
#include <TinyWireS.h>

// PORTB.3 (pin 2) is encoder pin 0
#define ENCODER0_bm _BV(PB3)
// PORTB.4 (pin 3) is encoder pin 1
#define ENCODER1_bm _BV(PB4)
// PORTB.1 (pin 6) is encoder click
#define ENCODER_SW_bm _BV(PB1)

#define I2C_ADDRESS 0x30

volatile uint8_t lastEncoderState = 0;
volatile int8_t encoderValue = 0;

ISR(PCINT0_vect) {
  uint8_t encoded = ((PINB & ENCODER0_bm) ? 1 : 0) + ((PINB & ENCODER1_bm) ? 2 : 0);
  uint8_t sum = (lastEncoderState << 2) | encoded;

  if(sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) {
    encoderValue += 2;
  }

  if(sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) {
    encoderValue -= 2;
  }

  lastEncoderState = encoded;

  if (PINB & ENCODER_SW_bm) {
    encoderValue |= 1;
  } else {
    encoderValue &= ~1;
  }
}

volatile uint8_t i2c_register;

void onReceive(uint8_t howMany) {
  // must be at least 1 but less than 16 bytes
  if (!howMany || howMany > 16) return;

  // recieve register
  i2c_register = TinyWireS.receive();
  howMany--;

  while (howMany) {
    TinyWireS.receive();
    howMany--;
    i2c_register++;
  }
}

void onRequest() {
  TinyWireS.send(encoderValue);
}

int main() {
  // setup i2c
  TinyWireS.begin(I2C_ADDRESS);
  TinyWireS.onReceive(onReceive);
  TinyWireS.onRequest(onRequest);

  // set encoder pins as input
  DDRB &= ~ENCODER0_bm;
  DDRB &= ~ENCODER1_bm;
  DDRB &= ~ENCODER_SW_bm;

  // enable internal pullup resistors
  PORTB |= ENCODER0_bm | ENCODER1_bm | ENCODER_SW_bm;

  // enable pin change interrupt
  GIMSK |= _BV(PCIE);
  // enable pin change interrupt on all encoder pins
  PCMSK |= _BV(PCINT3) | _BV(PCINT4) | _BV(PCINT1);

  // disable some peripherals to reduce power
  PRR |= _BV(PRTIM0) | _BV(PRTIM1) | _BV(PRADC);

  // enable interrupts
  sei();

  // loop doing nothing
  while (1) {
    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_mode();
  }
}
