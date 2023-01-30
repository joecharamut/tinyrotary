#include <stdint.h>
#include <string.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/io.h>

#include <avr/wdt.h>

#define __packed __attribute__((packed))
#define __noreturn __attribute__((noreturn))
#define __force_inline inline __attribute__((always_inline))

void __noreturn avr_reset() {
  wdt_enable(WDTO_30MS);
  while (1);
}

struct __packed UserRowConfig {
  uint8_t i2cAddress;
};
static_assert(sizeof(UserRowConfig) < USER_SIGNATURES_SIZE);

UserRowConfig config;

void loadConfig() {
  memcpy(&config, (void*)USER_SIGNATURES_START, sizeof(config));
}

void writeConfig() {
  // store prev interrupt state
  uint8_t interruptEnable = SREG & _BV(SREG_I);
  
  // wait while NVMCTRL is busy
  do {} while (NVMCTRL.STATUS & (NVMCTRL_EEBUSY_bm|NVMCTRL_FBUSY_bm));
  
  // disable interrupts
  cli();

  uint8_t *dest = reinterpret_cast<uint8_t *>(USER_SIGNATURES_START);
  uint8_t *src = reinterpret_cast<uint8_t *>(&config);
  // write config byte-wise
  for (uint8_t i = 0; i < sizeof(config) && i < USER_SIGNATURES_SIZE; i++) {
    // copy byte
    dest[i] = src[i];
    // call for page erase/write
    _PROTECTED_WRITE_SPM(NVMCTRL.CTRLA, NVMCTRL_CMD_PAGEERASEWRITE_gc);
    // wait until done
    do {} while (NVMCTRL.STATUS & (NVMCTRL_EEBUSY_bm|NVMCTRL_FBUSY_bm));
  }

  // restore interrupts if prev enabled
  if (interruptEnable) sei();
}

#define DEFAULT_I2C_ADDRESS 0x30
#define IS_VALID_I2C_ADDRESS(x) ((x) > 0x07 && (x) < 0x78)

volatile uint8_t i2c_register;

void onReceive(uint8_t howMany) {
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
}

#define ENCODER_A_bm _BV(PIN7)
#define ENCODER_B_bm _BV(PIN6)
#define ENCODER_SW_bm _BV(PIN3)

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
    
    : [encState] "+a" (encoderState)
    : [port] "I" (_SFR_IO_ADDR(VPORTA.IN))
  );
  
  // TODO: get this into assembly too
  encoderValue += ENCODER_STATE_TABLE[encoderState];

  asm (
    /* __tmp_reg__ = VPORTA.IN */
    "in __tmp_reg__, %[port]\n\t"
    /* T = __tmp_reg__(3) */
    "bst __tmp_reg__, 3\n\t"
    /* encoderSwitchState(0) = T */
    "bld %[swState], 0\n\t"

    : [swState] "+a" (encoderSwitchState)
    : [port] "I" (_SFR_IO_ADDR(VPORTA.IN))
  );
}

ISR(TWI0_TWIS_vect) {

}

int main() {
  // load config data from userrow
  loadConfig();

  // validate saved i2c address
  if (!IS_VALID_I2C_ADDRESS(config.i2cAddress)) {
    // if invalid, write default address and reset
    config.i2cAddress = DEFAULT_I2C_ADDRESS;
    writeConfig();
    avr_reset();
  }

  // setup pins as input
  // encoder pin A on PA7/pin 3
  // encoder pin B on PA6/pin 2
  // encoder click on PA3/pin 7
  PORTA.DIRCLR = _BV(PIN3) | _BV(PIN6) | _BV(PIN7);
  // enable pullup resistors and interrupt on both edges
  PORTA.PIN3CTRL = PORT_PULLUPEN_bm | PORT_ISC_BOTHEDGES_gc;
  PORTA.PIN6CTRL = PORT_PULLUPEN_bm | PORT_ISC_BOTHEDGES_gc;
  PORTA.PIN7CTRL = PORT_PULLUPEN_bm | PORT_ISC_BOTHEDGES_gc;

  // setup i2c
  // TWI0.SADDR = config.i2cAddress << 1;
  // TinyWireS.begin(I2C_ADDRESS);
  // TinyWireS.onReceive(onReceive);
  // TinyWireS.onRequest(onRequest);

  // enable pin change interrupt
  // GIMSK |= _BV(PCIE);
  // enable pin change interrupt on all encoder pins
  // PCMSK |= _BV(PCINT3) | _BV(PCINT4) | _BV(PCINT1);

  // disable some peripherals to reduce power
  // PRR |= _BV(PRTIM0) | _BV(PRTIM1) | _BV(PRADC);

  // enable interrupts
  // sei();

  // loop doing nothing
  while (1) {
    // set_sleep_mode(SLEEP_MODE_IDLE);
    // sleep_mode();
  }
}
