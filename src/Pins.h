#pragma once

// Encoder Port
#define ENCODER_PORT PORTA
// Encoder port interrupt
#define ENCODER_PORT_vect PORTA_PORT_vect
// Encoder A Pin: PA7/pin 3 
#define ENCODER_A_bp 7
// Encoder B Pin: PA6/pin 2
#define ENCODER_B_bp 6
// Encoder SW Pin: PA3/pin 7
#define ENCODER_S_bp 3

// Derive macros from pins above

// Encoder A Pin mask
#define ENCODER_A_bm _BV(ENCODER_A_bp)
// Encoder B Pin mask
#define ENCODER_B_bm _BV(ENCODER_B_bp)
// Encoder SW Pin mask
#define ENCODER_S_bm _BV(ENCODER_S_bp)

#define _ENCODER_CTRL_PASTE(x) PIN ## x ## CTRL
#define _ENCODER_CTRL_EVAL(x) _ENCODER_CTRL_PASTE(x)

// Encoder A Pin PINxCTRL
#define PIN_A_CTRL _ENCODER_CTRL_EVAL(ENCODER_A_bp)
// Encoder B Pin PINxCTRL
#define PIN_B_CTRL _ENCODER_CTRL_EVAL(ENCODER_B_bp)
// Encoder SW Pin PINxCTRL
#define PIN_S_CTRL _ENCODER_CTRL_EVAL(ENCODER_S_bp)

