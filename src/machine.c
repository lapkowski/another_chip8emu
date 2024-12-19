#include "machine.h"

#include "panic.h"
#include "backend.h"

#include <time.h>
#include <string.h>

Machine g_machine;

/* Stolen from https://tobiasvl.github.io/blog/write-a-chip-8-emulator/ */
uint8_t g_font[] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

void machine_load_rom(uint8_t* buffer)
{
    if (buffer == NULL) programming_error("API abuse detected: buffer == NULL. (in %s)", __func__);

    memcpy(g_machine.memory, g_font, sizeof(g_font));

    g_machine.program_counter = 0x200;
    memcpy(g_machine.memory+0x200, buffer, 4096-0x200);

    srand(time(NULL));
}

void machine_run_loop()
{
     uint16_t opcode = g_machine.memory[g_machine.program_counter] << 8 | g_machine.memory[g_machine.program_counter+1];
     uint8_t x, y, height, pixel;
     bool key_pressed = false;
     bool flag = false;

     switch (opcode & 0xF000) {
        case 0x0000:
             switch (opcode & 0x00FF) {
                 case 0x00E0: /*V 0x00E0: Clear the screen */
                     backend_clear_screen();
                     break;

                 case 0x00EE: /*V 0x00EE: Return from subroutine */
                     g_machine.stack_pointer--;
                     g_machine.program_counter = g_machine.stack[g_machine.stack_pointer];
                     break;

                 default:
                     panic("Invalid instruction: %#04x", (int)opcode);
                     break;
             }
             break;

        case 0x1000: /*V 0x1NNN: jump to NNN */
             g_machine.program_counter = (opcode & 0xFFF) - 2;
             break;

        case 0x2000: /*V 0x2NNN: Call a subroutine at NNN */
             g_machine.stack[g_machine.stack_pointer] = g_machine.program_counter;
             g_machine.stack_pointer++;
             if (g_machine.stack_pointer > sizeof(g_machine.stack)) panic("Stack overflow.");
             g_machine.program_counter = (opcode & 0xFFF) - 2;
             break;

        case 0x3000: /*V  0x3XNN: Skip the next instruction if vX equals NN */
             g_machine.program_counter += (g_machine.registers[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF)) * 2;
             break;

        case 0x4000: /*V 0x4XNN: Skip the next instruction if vX does not equal NN */
             g_machine.program_counter += (g_machine.registers[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF)) * 2;
             break;

        case 0x5000: /*V 0x4XY0: Skip the next instruction if vX equals vY */
             g_machine.program_counter += (g_machine.registers[(opcode & 0x0F00) >> 8] == g_machine.registers[(opcode & 0x00F0) >> 4]) * 2;
             break;

        case 0x6000: /*V 0x6XNN: set vX to NN */
             g_machine.registers[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
             break;

        case 0x7000: /*V 0x7XNN: add NN to vX */
              g_machine.registers[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
              break;

        case 0x8000:
              switch (opcode & 0x000F) {
                  case 0x0000: /* 0x8XY0: Set vX to the value of vY */
                      g_machine.registers[(opcode & 0x0F00) >> 8] = g_machine.registers[(opcode & 0x00F0) >> 4];
                      break;

                  case 0x0001: /* 0x8XY1: Set vX to the value of (vX | vY) */
                      g_machine.registers[(opcode & 0x0F00) >> 8] |= g_machine.registers[(opcode & 0x00F0) >> 4];
                      break;

                  case 0x0002: /* 0x8XY2: Set vX to the value of (vX & vY) */
                      g_machine.registers[(opcode & 0x0F00) >> 8] &= g_machine.registers[(opcode & 0x00F0) >> 4];
                      break;

                  case 0x0003: /* 0x8XY3: Set vX to the value of (vY ^ vY) */
                      g_machine.registers[(opcode & 0x0F00) >> 8] ^= g_machine.registers[(opcode & 0x00F0) >> 4];
                      break;

                  case 0x0004: /* 0x8XY4: Add vY to vX. Use vF as the carry flag. */
                      flag = g_machine.registers[(opcode & 0x0F00) >> 8] + g_machine.registers[(opcode & 0x00F0) >> 4] > 0xFF;
                      g_machine.registers[(opcode & 0x0F00) >> 8] += g_machine.registers[(opcode & 0x00F0) >> 4];
                      g_machine.registers[0xF] = flag;
                      break;

                  case 0x0005: /* 0x8XY5: Subtract vY from vX. Use vF as the borrow flag */
                      flag = g_machine.registers[(opcode & 0x0F00) >> 8] - g_machine.registers[(opcode & 0x00F0) >> 4] >= 0;
                      g_machine.registers[(opcode & 0x0F00) >> 8] -= g_machine.registers[(opcode & 0x00F0) >> 4];
                      g_machine.registers[0xF] = flag;
                      break;

                  case 0x0006: /* 0x8XY6: Shift vX to the right by one. Set vF to the bit lost by shifting !!! */
                      flag = (g_machine.registers[(opcode & 0x00F0) >> 4] >> 0) & 0x01;
                      g_machine.registers[(opcode & 0x0F00) >> 8] = g_machine.registers[(opcode & 0x00F0) >> 4] >> 1;
                      g_machine.registers[0xF] = flag;
                      break;

                  case 0x0007: /* 0x8XY5: Subtract vX from vY. Store the value in vX. Use vF as the borrow flag */
                      flag = g_machine.registers[(opcode & 0x00F0) >> 4] - g_machine.registers[(opcode & 0x0F00) >> 8] >= 0;
                      g_machine.registers[(opcode & 0x0F00) >> 8] = g_machine.registers[(opcode & 0x00F0) >> 4] - g_machine.registers[(opcode & 0x0F00) >> 8];
                      g_machine.registers[0xF] = flag;
                      break;

                  case 0x000E: /* 0x8XY6: Shift vX to the left by one. Set vF to the bit lost by shifting !!! */
                      flag = (g_machine.registers[(opcode & 0x00F0) >> 4] >> 7) & 0x01;
                      g_machine.registers[(opcode & 0x0F00) >> 8] = g_machine.registers[(opcode & 0x00F0) >> 4] << 1;
                      g_machine.registers[0xF] = flag;
                      break;

                  default:
                      panic("Invalid instruction: %#04x", (int)opcode);
                      break;
              };
              break;

        case 0x9000: /* 0x9XY0: Skip the next instruction if vX does not equal vY */
             g_machine.program_counter += (g_machine.registers[(opcode & 0x0F00) >> 8] != g_machine.registers[(opcode & 0x00F0) >> 4]) * 2;
             break;

        case 0xA000: /* 0xANNN: set I to NNN */
              g_machine.index_register = opcode & 0x0FFF;
              break;

        case 0xB000: /* 0xBNNN: Jump to the address NNN plus v0 */
              g_machine.program_counter = (opcode & 0x0FFF) + g_machine.registers[0] - 2;
              break;

        case 0xC000: /* 0xCXNN: Set vX to a random number masked by NN */
              g_machine.registers[(opcode & 0x0F00) >> 8] = (rand() % (0xFF + 1)) & (opcode & 0x00FF);
              break;

        case 0xD000: /* 0xDYXN: Draw sprite at I to the location at vX and vY that is N pixels tall */
              x = g_machine.registers[(opcode & 0x0F00) >> 8] % 64;
              y = g_machine.registers[(opcode & 0x00F0) >> 4] % 32;
              height = opcode & 0x000F;

              g_machine.registers[0xF] &= 0;

              for (int yline = 0; yline < height; yline++) {
                    pixel = g_machine.memory[g_machine.index_register + yline]; 

                    for (int xline = 0; xline < 8; xline++) {
                        if (!(pixel & (0x80 >> xline))) continue;

                        g_machine.registers[0xF] |= backend_flip_pixel(x + xline, y + yline);
                    }
              }
              break;

        case 0xE000:
              switch (opcode & 0x00FF) {
                  case 0x009E: /* 0xEX9E: Skips the next instruction if the key stored in vX is pressed */
                      if (backend_is_pressed((Chip8Key)g_machine.registers[(opcode & 0x0F00) >> 8])) g_machine.program_counter += 2;
                      break;

                  case 0x00A1: /* 0xEXA1: Skips the next instruction if the key stored in vX isn't pressed */
                      if (!backend_is_pressed((Chip8Key)g_machine.registers[(opcode & 0x0F00) >> 8])) g_machine.program_counter += 2;
                      break;

                  default:
                      panic("Invalid instruction: %#04x", (int)opcode);
                      break;
              };
              break;

        case 0xF000:
              switch (opcode & 0x00FF) {
                  case 0x0007: /* 0xFX07: Set vX to the value of the delay timer */
                      g_machine.registers[(opcode & 0x0F00) >> 8] = g_machine.delay_timer;
                      break;

                  case 0x000A: /* 0xFX00: Wait for a keypress, and store it in vX */
                      for (int i=0; i<16; i++) {
                          if (!backend_is_pressed((Chip8Key)i)) continue;

                          g_machine.registers[(opcode & 0x0F00) >> 8] = i;
                          key_pressed = true;
                      }

                      if (!key_pressed) {
                          g_machine.program_counter -= 2;
                          break;
                      }
                      break;
                 
                  case 0x0015: /* 0xFX15: Set the delay timer to vX */
                      g_machine.delay_timer = g_machine.registers[(opcode & 0x0F00) >> 8];
                      break;

                  case 0x0018: /* 0xFX18: Set the sound timer to vX */
                      g_machine.sound_timer = g_machine.registers[(opcode & 0x0F00) >> 8];
                      break;

                  case 0x001E: /* 0xFX1E: Add vX to I. */
                      g_machine.index_register += g_machine.registers[(opcode & 0x0F00) >> 8];
                      break;

                  case 0x0029: /* 0xFX29: Set I to the location of the sprite for the character X in the font */
                      g_machine.index_register = g_machine.registers[(opcode & 0x0F00) >> 8] * 5; 
                      break;

                  case 0x0033: /* 0xFX33 - Store the Binary-coded decimal reprezentation of vX at addresses I, I+1, and I+3 */
                      g_machine.memory[g_machine.index_register]   = g_machine.registers[(opcode & 0x0F00) >> 8] / 100;
                      g_machine.memory[g_machine.index_register+1] = (g_machine.registers[(opcode & 0x0F00) >> 8] / 10) % 10;
                      g_machine.memory[g_machine.index_register+2] = g_machine.registers[(opcode & 0x0F00) >> 8] % 10;
                      break;

                  case 0x0055: /* 0xFX55: Store value to vX in memory starting at address I */
                      for (int i=0; i <= ((opcode & 0x0F00) >> 8); i++) {
                          g_machine.memory[g_machine.index_register + i] = g_machine.registers[i];
                      }

                      g_machine.index_register += ((opcode & 0x0F00) >> 8) + 1;
                      break;

                  case 0x0065: /* 0xFX65: Load value to vX in memory starting at address I */
                      for (int i=0; i <= ((opcode & 0x0F00) >> 8); i++) {
                          g_machine.registers[i] = g_machine.memory[g_machine.index_register + i];
                      }

                      g_machine.index_register += ((opcode & 0x0F00) >> 8) + 1;
                      break;


                  default:
                      panic("Invalid instruction: %#04x", (int)opcode);
                      break;
              };
              break;

        default:
            panic("Invalid instruction: %#04x", (int)opcode);
            break;
     };

     g_machine.program_counter += 2;

     if (g_machine.delay_timer > 0) g_machine.delay_timer--;

     if (g_machine.sound_timer > 0) {
        g_machine.sound_timer--;
        backend_toggle_beep(true);
     } else {
        backend_toggle_beep(false);
     }
}
