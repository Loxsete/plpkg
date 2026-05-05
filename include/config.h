#pragma once

#include <stddef.h>

#define ASD_CONF_PATH "/etc/asd/asd.conf"
#define ASD_CONF_LOCAL_PATH "/etc/asd/asd.conf.local"

typedef struct {
    char repo_user[128];
    char repo_name[128];
    char db_dir[256];
    char tmp_dir[256];
    char build_dir[256];
    int  verbose;
    int  color;
} AsdConfig;

extern AsdConfig g_cfg;

int  config_load(void);
void config_dump(void);