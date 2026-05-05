#include "utils.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

void strip(char *s) {
    int len = (int)strlen(s);
    while (len > 0 && (s[len-1] == '\n' || s[len-1] == '\r' || s[len-1] == ' '))
        s[--len] = '\0';
    char *p = s;
    while (*p == ' ') p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
}

int file_exists(const char *path) {
    return access(path, F_OK) == 0;
}

void ensure_dir(const char *path) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "mkdir -p %s", path);
    (void)system(cmd);
}

void asd_print(const char *tag, const char *color, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    if (g_cfg.color)
        fprintf(stdout, "%s%s%s %s%s ",
            ANSI_BOLD, color, tag, ANSI_RESET, ANSI_WHITE);
    else
        fprintf(stdout, "%s ", tag);

    vfprintf(stdout, fmt, ap);

    if (g_cfg.color)
        fprintf(stdout, "%s\n", ANSI_RESET);
    else
        fprintf(stdout, "\n");

    va_end(ap);
}