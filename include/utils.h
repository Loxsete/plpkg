#pragma once

#include <stddef.h>

#define ANSI_RESET   "\033[0m"
#define ANSI_BOLD    "\033[1m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_CYAN    "\033[36m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_RED     "\033[31m"
#define ANSI_TEAL    "\033[38;5;37m"
#define ANSI_GRAY    "\033[38;5;240m"
#define ANSI_WHITE   "\033[97m"

void strip(char *s);
int  file_exists(const char *path);
void ensure_dir(const char *path);

void asd_print(const char *tag, const char *color, const char *fmt, ...);

#define msg_ok(...)   asd_print(">>>", ANSI_GREEN,  __VA_ARGS__)
#define msg_info(...) asd_print("---", ANSI_TEAL,   __VA_ARGS__)
#define msg_warn(...) asd_print("!!!", ANSI_YELLOW, __VA_ARGS__)
#define msg_err(...)  asd_print("***", ANSI_RED,    __VA_ARGS__)
#define msg_step(...) asd_print("   ", ANSI_GRAY,   __VA_ARGS__)