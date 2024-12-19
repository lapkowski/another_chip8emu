#pragma once

#include <stdint.h>

/*
 Chip8 specyfications:
    - 4Kb of memory
    - 64x32 (or 128x64) screen
    - A program counter (word)
    - One 16-bit index register called “I” which is used to point at locations in memory
    - A stack for 16-bit addresses, which is used to call subroutines/functions and return from them
    - An 8-bit delay timer which is decremented at a rate of 60 Hz (60 times per second) until it reaches 0
    - An 8-bit sound timer which functions like the delay timer, but which also gives off a beeping sound as long as it’s not 0
    - 16 8-bit (one byte) general-purpose variable registers numbered 0 through F hexadecimal, ie. 0 through 15 in decimal, called V0 through VF
*/

typedef struct {
  uint8_t  memory[4096];
  uint8_t  screen[64 * 32 / 8];
  uint16_t program_counter;
  uint16_t index_register;
  uint16_t stack[16];
  uint8_t  stack_pointer;
  uint8_t  delay_timer;
  uint8_t  sound_timer;
  uint8_t  registers[16];
} Machine;

extern Machine g_machine;

void machine_load_rom(uint8_t* buffer);
void machine_run_loop();
