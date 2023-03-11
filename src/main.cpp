#include <stdint.h>
#include <string.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/io.h>

#include <avr/wdt.h>

#include "Config.h"
#include "Pins.h"

#include <Arduino.h>
#include <Wire.h>

#define __packed __attribute__((packed))
#define __noreturn __attribute__((noreturn))
#define __force_inline inline __attribute__((always_inline))

void __noreturn avr_reset() {
  wdt_enable(WDTO_30MS);
  while (1);
}

#define DEFAULT_I2C_ADDRESS 0x30
#define IS_VALID_I2C_ADDRESS(x) ((x) > 0x07 && (x) < 0x78)

#define ENCODER_STEP_SIZE 2
const int8_t ENCODER_STATE_TABLE[16] = {
  /* 0b0000 */  0,
  /* 0b0001 */ -ENCODER_STEP_SIZE,
  /* 0b0010 */ +ENCODER_STEP_SIZE,
  /* 0b0011 */  0,
  /* 0b0100 */ +ENCODER_STEP_SIZE,
  /* 0b0101 */  0,
  /* 0b0110 */  0,
  /* 0b0111 */ -ENCODER_STEP_SIZE,
  /* 0b1000 */ -ENCODER_STEP_SIZE,
  /* 0b1001 */  0,
  /* 0b1010 */  0,
  /* 0b1011 */ +ENCODER_STEP_SIZE,
  /* 0b1100 */  0,
  /* 0b1101 */ +ENCODER_STEP_SIZE,
  /* 0b1110 */ -ENCODER_STEP_SIZE,
  /* 0b1111 */  0,
};

volatile uint8_t encoderState;
volatile uint8_t encoderValue;
volatile uint8_t encoderSwitchState;
ISR(PORTA_PORT_vect) {
  // clear int flags
  VPORTA.INTFLAGS = ENCODER_A_bm | ENCODER_B_bm | ENCODER_S_bm;
  // read in current encoder state
  asm (
    /* encoderState <<= 2 */
    "lsl %[encState]\n\t"
    "lsl %[encState]\n\t"

    /* encoderState &= 0xF */
    "andi %[encState], 0xF\n\t"

    /* __tmp_reg__ = VPORTA.IN */
    "in __tmp_reg__, %[port]\n\t"
    
    /* SREG.T = 1 */
    "set\n\t"

    /* if (__tmp_reg__ & (1<<6)) encoderState |= (SREG.T<<0) */
    "sbrc __tmp_reg__, 6\n\t"
      "bld %[encState], 0\n\t"
    
    /* if (__tmp_reg__ & (1<<7)) encoderState |= (SREG.T<<1) */
    "sbrc __tmp_reg__, 7\n\t"
      "bld %[encState], 1\n\t"
    
    /* ZL += encoderState */
    "add r30, %[encState]\n\t"
    /* ZH += SREG.C */
    "adc r31, __zero_reg__\n\t"

    /* __tmp_reg__ = *((uint8_t*) Z) */
    "ld __tmp_reg__, Z\n\t"
    /* encoderValue += __tmp_reg__ */
    "add %[encValue], __tmp_reg__\n\t"

    /* __tmp_reg__ = VPORTA.IN */
    "in __tmp_reg__, %[port]\n\t"
    /* T = __tmp_reg__(3) */
    "bst __tmp_reg__, 3\n\t"
    /* encoderSwitchState(0) = T */
    "bld %[swState], 0\n\t"
    
    : [encState] "+a" (encoderState), [encValue] "+a" (encoderValue), [swState] "+a" (encoderSwitchState)
    : [port] "I" (_SFR_IO_ADDR(VPORTA.IN)), [stateTable] "z" (ENCODER_STATE_TABLE)
  );
}

volatile uint8_t i2c_register;
void onReceive(int howMany) {
  // must be at least 1 but less than 16 bytes
  if (!howMany || howMany > 16) return;

  // recieve register
  // i2c_register = TinyWireS.receive();
  howMany--;

  while (howMany) {
    // TinyWireS.receive();
    howMany--;
    i2c_register++;
  }
}

void onRequest() {
  // TinyWireS.send(encoderValue);
  Wire.write(encoderValue);
}

void i2cRequest() {

}

void i2cReceive() {

}

uint8_t i2c_buf;
// ISR(TWI0_TWIS_vect) {
//   // read address from data register
//   i2c_buf = TWI0.SDATA;

//   // check operation
//   if (TWI0.SSTATUS & TWI_DIR_bm) {
//     // if direction bit set, bus controller trying to read data
//     // need to reply with something
//     // TODO: configurable whether to pack switch state into encoder position?
//     TWI0.SDATA = 0;
//     TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
//   } else {
//     // dir bit clear: bus controller trying to write data
    
//   }
// }

int main() {
  // load config data from userrow
  // CONFIG.load();

  // validate saved i2c address
  // if (!IS_VALID_I2C_ADDRESS(CONFIG.i2cAddress)) {
  //   // if invalid, write default address and reset
  //   CONFIG.i2cAddress = DEFAULT_I2C_ADDRESS;
  //   CONFIG.save();
  //   avr_reset();
  // }

  // setup pins as input
  ENCODER_PORT.DIRCLR = ENCODER_A_bm | ENCODER_B_bm | ENCODER_S_bm;

  // enable pullup resistors and interrupt on both edges
  ENCODER_PORT.PIN_A_CTRL = PORT_PULLUPEN_bm | PORT_ISC_BOTHEDGES_gc;
  ENCODER_PORT.PIN_B_CTRL = PORT_PULLUPEN_bm | PORT_ISC_BOTHEDGES_gc;
  ENCODER_PORT.PIN_S_CTRL = PORT_PULLUPEN_bm | PORT_ISC_BOTHEDGES_gc;

  // setup i2c
  // set device address
  // TWI0.SADDR = DEFAULT_I2C_ADDRESS << 1;
  // // configure peripheral
  // TWI0.SCTRLA = 0
  //   | TWI_DIEN_bm     // data interrupt enable 
  //   | TWI_APIEN_bm    // address interrupt enable
  //   // | TWI_PIEN_bm     // no stop interrupt
  //   // | TWI_PMEN_bm     // no promiscuious mode
  //   | TWI_SMEN_bm     // smart mode enable
  //   | TWI_ENABLE_bm;  // enable

  Wire.onReceive(onReceive);
  Wire.onRequest(onRequest);
  Wire.begin(DEFAULT_I2C_ADDRESS);

  // enable interrupts
  sei();

  // loop doing nothing
  while (1) {
    // set_sleep_mode(SLEEP_MODE_IDLE);
    // sleep_mode();
  }
}
