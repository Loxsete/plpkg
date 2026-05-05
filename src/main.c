#include "config.h"
#include "utils.h"
#include "package.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/wait.h>

static void print_banner(void) {
    if (!g_cfg.color) return;
    printf("%s%s", ANSI_TEAL, ANSI_BOLD);
    printf("  __ _ ___  __| |\n");
    printf(" / _` / __|/ _` |\n");
    printf("| (_| \\__ \\ (_| |\n");
    printf(" \\__,_|___/\\__,_|   %s%spackage manager%s\n\n",
        ANSI_RESET, ANSI_GRAY, ANSI_RESET);
}

static void separator(void) {
    if (g_cfg.color)
        printf("%s%s=================================================%s\n",
            ANSI_GRAY, ANSI_BOLD, ANSI_RESET);
    else
        printf("=================================================\n");
}

static int cmd_install(const char *pkgname) {
    separator();
    msg_ok("Installing: %s%s%s", ANSI_CYAN, pkgname, ANSI_RESET);
    separator();

    if (pkg_is_installed(pkgname)) {
        msg_warn("%s is already installed -- skipping", pkgname);
        return 0;
    }

    if (pkg_fetch_repo() != 0) return -1;

    char pkg_path[2048];
    if (pkg_find_path(pkgname, pkg_path, sizeof(pkg_path)) != 0) {
        msg_err("No package named '%s' found in %s/%s",
            pkgname, g_cfg.repo_user, g_cfg.repo_name);
        return -1;
    }

    Package pkg;
    memset(&pkg, 0, sizeof(pkg));
    strncpy(pkg.name, pkgname, sizeof(pkg.name) - 1);

    char info_path[2048];
    snprintf(info_path, sizeof(info_path), "%s/info", pkg_path);
    if (pkg_parse_info(info_path, &pkg) != 0) return -1;

    char deps_path[2048];
    snprintf(deps_path, sizeof(deps_path), "%s/deps", pkg_path);
    pkg_parse_deps(deps_path, &pkg);

    for (int i = 0; i < pkg.dep_count; i++) {
        if (!pkg_is_installed(pkg.deps[i])) {
            if (cmd_install(pkg.deps[i]) != 0) return -1;
        }
    }

    char make_path[2048];
    snprintf(make_path, sizeof(make_path), "%s/make.sh", pkg_path);

    if (!file_exists(make_path)) {
        msg_err("make.sh not found for %s", pkgname);
        return -1;
    }

    msg_ok("Building %s-%s ...", pkg.name, pkg.version);
    separator();

    pid_t pid = fork();

    if (pid == 0) {
        setenv("PKG_NAME", pkg.name, 1);
        setenv("PKG_VERSION", pkg.version, 1);
        setenv("PKG_CATEGORY", pkg.category, 1);

        setenv("ASD_PREFIX", "/usr", 1);

        char jobs[16];
        snprintf(jobs, sizeof(jobs), "%ld", sysconf(_SC_NPROCESSORS_ONLN));
        setenv("ASD_JOBS", jobs, 1);

        execl("/bin/bash", "bash", make_path, NULL);
        exit(127);
    }
    else if (pid < 0) {
        return -1;
    }

    int status;
    waitpid(pid, &status, 0);
    int ret = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

    if (ret != 0) {
        msg_err("Build failed for %s", pkgname);
        return -1;
    }

    pkg_mark_installed(&pkg);
    msg_ok("%s-%s merged successfully.", pkg.name, pkg.version);
    separator();
    return 0;
}

static int cmd_remove(const char *pkgname) {
    if (!pkg_is_installed(pkgname)) return -1;
    if (pkg_mark_removed(pkgname) != 0) return -1;
    return 0;
}

static int cmd_list(void) {
    DIR *d = opendir(g_cfg.db_dir);
    if (!d) return 0;

    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        printf("%s\n", entry->d_name);
    }

    closedir(d);
    return 0;
}

static int cmd_info(const char *pkgname) {
    if (!pkg_is_installed(pkgname)) return -1;

    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", g_cfg.db_dir, pkgname);

    FILE *f = fopen(path, "r");
    if (!f) return -1;

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        printf("%s", line);
    }

    fclose(f);
    return 0;
}

int main(int argc, char *argv[]) {
    config_load();

    if (argc < 2) return 1;

    const char *cmd = argv[1];

    if (strcmp(cmd, "install") == 0) {
        if (argc < 3) return 1;
        return cmd_install(argv[2]);
    } else if (strcmp(cmd, "remove") == 0) {
        if (argc < 3) return 1;
        return cmd_remove(argv[2]);
    } else if (strcmp(cmd, "list") == 0) {
        return cmd_list();
    } else if (strcmp(cmd, "info") == 0) {
        if (argc < 3) return 1;
        return cmd_info(argv[2]);
    }

    return 1;
}
