#include "config.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

AsdConfig g_cfg = {
    .repo_user  = "YourGitHubUser",
    .repo_name  = "asd-packages",
    .db_dir     = "/var/lib/asd/installed",
    .tmp_dir    = "/tmp/asd",
    .build_dir  = "/tmp/asd/build",
    .verbose    = 0,
    .color      = 1,
};

static void apply_key(const char *key, const char *val) {
    if      (strcmp(key, "repo_user")  == 0) strncpy(g_cfg.repo_user,  val, sizeof(g_cfg.repo_user)  - 1);
    else if (strcmp(key, "repo_name")  == 0) strncpy(g_cfg.repo_name,  val, sizeof(g_cfg.repo_name)  - 1);
    else if (strcmp(key, "db_dir")     == 0) strncpy(g_cfg.db_dir,     val, sizeof(g_cfg.db_dir)     - 1);
    else if (strcmp(key, "tmp_dir")    == 0) strncpy(g_cfg.tmp_dir,    val, sizeof(g_cfg.tmp_dir)    - 1);
    else if (strcmp(key, "build_dir")  == 0) strncpy(g_cfg.build_dir,  val, sizeof(g_cfg.build_dir)  - 1);
    else if (strcmp(key, "verbose")    == 0) g_cfg.verbose = atoi(val);
    else if (strcmp(key, "color")      == 0) g_cfg.color   = atoi(val);
}

static int parse_conf(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        strip(line);
        if (line[0] == '#' || line[0] == '\0') continue;

        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = line;
        char *val = eq + 1;
        strip(key);
        strip(val);
        apply_key(key, val);
    }

    fclose(f);
    return 0;
}

int config_load(void) {
    parse_conf(ASD_CONF_PATH);
    parse_conf(ASD_CONF_LOCAL_PATH);
    return 0;
}

void config_dump(void) {
    printf("repo_user  = %s\n", g_cfg.repo_user);
    printf("repo_name  = %s\n", g_cfg.repo_name);
    printf("db_dir     = %s\n", g_cfg.db_dir);
    printf("tmp_dir    = %s\n", g_cfg.tmp_dir);
    printf("build_dir  = %s\n", g_cfg.build_dir);
    printf("verbose    = %d\n", g_cfg.verbose);
    printf("color      = %d\n", g_cfg.color);
}