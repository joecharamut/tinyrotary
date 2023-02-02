#include "Config.h"

#include <string.h>
#include <avr/interrupt.h>

UserRowConfig CONFIG;

void UserRowConfig::load() {
  memcpy(&CONFIG, (void*)USER_SIGNATURES_START, sizeof(UserRowConfig));
}

static void userrow_write_byte(uint8_t offset, uint8_t byte) {
  if (offset >= USER_SIGNATURES_SIZE) return;

  // store prev interrupt state
  uint8_t interruptEnable = SREG & _BV(SREG_I);
  
  // wait while NVMCTRL is busy
  do {} while (NVMCTRL.STATUS & (NVMCTRL_EEBUSY_bm|NVMCTRL_FBUSY_bm));
  
  // disable interrupts
  cli();

  // write byte to page buf
  *((uint8_t *) (USER_SIGNATURES_START + offset)) = byte;
  // call for page erase/write
  _PROTECTED_WRITE_SPM(NVMCTRL.CTRLA, NVMCTRL_CMD_PAGEERASEWRITE_gc);
  // wait until done
  do {} while (NVMCTRL.STATUS & (NVMCTRL_EEBUSY_bm|NVMCTRL_FBUSY_bm));

  // restore interrupts if prev enabled
  if (interruptEnable) sei();
}

void UserRowConfig::save() {
  uint8_t *configBytes = (uint8_t *) &CONFIG;

  // write config byte-wise
  for (uint8_t i = 0; i < sizeof(UserRowConfig); i++) {
    userrow_write_byte(i, configBytes[i]);
  }
}


