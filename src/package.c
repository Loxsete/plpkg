#include "package.h"
#include "config.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

int pkg_parse_info(const char *path, Package *pkg) {
    FILE *f = fopen(path, "r");
    if (!f) {
        msg_err("cannot open info: %s", path);
        return -1;
    }

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        strip(line);
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key = line;
        char *val = eq + 1;

        if      (strcmp(key, "version")    == 0) strncpy(pkg->version,    val, sizeof(pkg->version)    - 1);
        else if (strcmp(key, "developer")  == 0) strncpy(pkg->developer,  val, sizeof(pkg->developer)  - 1);
        else if (strcmp(key, "maintainer") == 0) strncpy(pkg->maintainer, val, sizeof(pkg->maintainer) - 1);
        else if (strcmp(key, "category")   == 0) strncpy(pkg->category,   val, sizeof(pkg->category)   - 1);
    }

    fclose(f);
    return 0;
}

int pkg_parse_deps(const char *path, Package *pkg) {
    pkg->dep_count = 0;
    FILE *f = fopen(path, "r");
    if (!f) return 0;

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        strip(line);
        if (strlen(line) == 0) continue;
        if (pkg->dep_count >= MAX_DEPS) {
            msg_warn("too many deps, truncating at %d", MAX_DEPS);
            break;
        }
        strncpy(pkg->deps[pkg->dep_count++], line, sizeof(pkg->deps[0]) - 1);
    }

    fclose(f);
    return 0;
}

int pkg_fetch_repo(void) {
    ensure_dir(g_cfg.tmp_dir);

    char cmd[2048];
    snprintf(cmd, sizeof(cmd),
        "curl -sL https://github.com/%s/%s/archive/refs/heads/main.tar.gz -o %s/repo.tar.gz",
        g_cfg.repo_user, g_cfg.repo_name, g_cfg.tmp_dir);

    msg_info("Fetching %s/%s ...", g_cfg.repo_user, g_cfg.repo_name);
    if (system(cmd) != 0) {
        msg_err("curl failed");
        return -1;
    }

    snprintf(cmd, sizeof(cmd), "tar -xzf %s/repo.tar.gz -C %s", g_cfg.tmp_dir, g_cfg.tmp_dir);
    if (system(cmd) != 0) {
        msg_err("tar failed");
        return -1;
    }

    return 0;
}

int pkg_find_path(const char *pkgname, char *out_path, size_t out_size) {
    char repo_root[2048];
    snprintf(repo_root, sizeof(repo_root), "%s/%s-main", g_cfg.tmp_dir, g_cfg.repo_name);

    DIR *root = opendir(repo_root);
    if (!root) {
        msg_err("cannot open repo root: %s", repo_root);
        return -1;
    }

    struct dirent *entry;
    while ((entry = readdir(root)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        char cat_path[2048];
        snprintf(cat_path, sizeof(cat_path), "%s/%s", repo_root, entry->d_name);

        struct stat st;
        if (stat(cat_path, &st) != 0 || !S_ISDIR(st.st_mode)) continue;

        char pkg_path[2048];
        snprintf(pkg_path, sizeof(pkg_path), "%s/%s", cat_path, pkgname);

        if (file_exists(pkg_path)) {
            strncpy(out_path, pkg_path, out_size - 1);
            out_path[out_size - 1] = '\0';
            closedir(root);
            return 0;
        }
    }

    closedir(root);
    return -1;
}

int pkg_is_installed(const char *pkgname) {
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", g_cfg.db_dir, pkgname);
    return file_exists(path);
}

void pkg_mark_installed(Package *pkg) {
    ensure_dir(g_cfg.db_dir);
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", g_cfg.db_dir, pkg->name);

    FILE *f = fopen(path, "w");
    if (!f) {
        msg_err("cannot write db entry for %s", pkg->name);
        return;
    }
    fprintf(f, "version=%s\n",    pkg->version);
    fprintf(f, "developer=%s\n",  pkg->developer);
    fprintf(f, "maintainer=%s\n", pkg->maintainer);
    fprintf(f, "category=%s\n",   pkg->category);
    fclose(f);
}

int pkg_mark_removed(const char *pkgname) {
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", g_cfg.db_dir, pkgname);
    return remove(path);
}