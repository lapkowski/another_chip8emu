#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "machine.h"
#include "panic.h"
#include "backend.h"

bool onquit()
{
    backend_destroy();
    return true;
}

void load_rom(char** argv) {
    FILE* rom_file = fopen(argv[1], "rb");
    if (rom_file == NULL) panic("File %s could not be read: %s", argv[1], strerror(errno));

    uint8_t rom[4096-0x200] = { 0 };
    fread(rom, 4096-0x200, 1, rom_file);
    fclose(rom_file);

    machine_load_rom(rom);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: %s [rom]", argv[0]);
        exit(0);
    }

    load_rom(argv);
    set_self_destruct_handler(onquit);
    backend_initialize();

    while(!backend_loop()) {
        machine_run_loop();
        backend_delay(6);
    }

    return !onquit();
}
