#include <SDL2/SDL.h>

#include <stdarg.h>
#include <stdbool.h>

#define PROJECT_NAME "another_chip8emu"
#define PROJECT_REPO "https://github.com/lapkowski/another_chip8emu"

bool (*self_destruct)() = NULL;

void set_self_destruct_handler(bool (*c)())
{
    self_destruct = c;
}

__attribute__((noreturn)) void panic(const char* msg, ...)
{
    va_list arguments;
    va_start(arguments, msg);

    fprintf(stderr, "The program panicked with the following message: \"");
    vfprintf(stderr, msg, arguments);
    fprintf(stderr, "\"\n");

    va_end(arguments);

    if (!self_destruct) exit(1);

    fprintf(stderr, "Running self destruct handlers... ");
    fprintf(stderr, "[%s]\n", self_destruct() ? " OK " : "Fail");

    exit(1);
}

__attribute__((noreturn)) void programming_error(const char* msg, ...)
{
    va_list arguments;
    va_start(arguments, msg);

    fprintf(stderr, "The program panicked with the following message: \"");
    vfprintf(stderr, msg, arguments);
    fprintf(stderr, "\"\n");

    va_end(arguments);
    fprintf(stderr, "\n[This is a programming oversight, not a problem with your setup]\n"
                    "To help us resolve the problem proceed with the folowing steps:\n"
                    "  1. Check the current version of " PROJECT_NAME " and update it if needed.\n"
                    "  2. If the problem persists, send the error message at " PROJECT_REPO " along with anything that could influence the execution (configuration files, emulated rom, etc.).\n"
                    "  3. Stay in touch with us (just check the issue every now and then).\n"
                    "  4. Wait for the next update!\n\n"
                    "Thank you for helping us with the development of " PROJECT_NAME " <3\n");

    if (!self_destruct) exit(1);

    fprintf(stderr, "Running self destruct handlers... ");
    fprintf(stderr, "[%s]\n", self_destruct() ? " OK " : "Fail");

    exit(1);
}

__attribute__((noreturn)) void todo()
{
    panic("Not yet implemented");
}
