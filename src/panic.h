#pragma once

#include <stdbool.h>
#include <stdlib.h>

#define API_ABUSE_WHEN(x) if (__builtin_expect(x, 0)) programming_error("Api abuse on %s: " #x ".", __func__)
#define API_ABUSE_WHEN_CUSTOM(x, msg) if (__builtin_expect(x, 0)) programming_error("Api abuse on %s: " msg ".", __func__)

extern bool (*self_destruct)();

void set_self_destruct_handler(bool (*c)());

__attribute__((noreturn)) void panic(const char *msg, ...);
__attribute__((noreturn)) void programming_error(const char *msg, ...);
__attribute__((noreturn)) void todo();
