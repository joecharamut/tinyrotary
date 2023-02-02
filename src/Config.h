#pragma once

#include <stdint.h>
#include <avr/io.h>

#define __packed __attribute__((packed))

struct __packed UserRowConfig {
  uint8_t i2cAddress;

  static void load();
  static void save();
};
static_assert(sizeof(UserRowConfig) < USER_SIGNATURES_SIZE);


extern UserRowConfig CONFIG;
