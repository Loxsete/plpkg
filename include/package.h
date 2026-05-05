#pragma once
#include <stddef.h>

#define MAX_LINE  512
#define MAX_DEPS  64

typedef struct {
    char name[128];
    char version[64];
    char developer[128];
    char maintainer[128];
    char category[128];
    char deps[MAX_DEPS][128];
    int  dep_count;
} Package;

int  pkg_parse_info(const char *path, Package *pkg);
int  pkg_parse_deps(const char *path, Package *pkg);
int  pkg_fetch_repo(void);
int  pkg_find_path(const char *pkgname, char *out_path, size_t out_size);
int  pkg_is_installed(const char *pkgname);
void pkg_mark_installed(Package *pkg);
int  pkg_mark_removed(const char *pkgname);
