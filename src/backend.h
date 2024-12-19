#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    KEY_0 = 0,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_A,
    KEY_B,
    KEY_C,
    KEY_D,
    KEY_E,
    KEY_F
} Chip8Key;

/* Infalliable, will panic on error */
void backend_initialize();
/* Run inside a while loop, returns true if a loop continues */
bool backend_loop();
/* Infalliable, will panic on error */
void backend_destroy();

/* Will always return a non-null pointer, if initialized */
uint8_t** backend_get_pixel_map();
bool backend_flip_pixel(uint8_t x, uint8_t y);
void backend_clear_screen();

void backend_toggle_beep(bool beep);

bool backend_is_pressed(Chip8Key key);

void backend_delay(uint32_t ms);
