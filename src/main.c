#include "config.h"
#include "utils.h"
#include "package.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/wait.h>

static int cmd_help() {
	printf("To use asd type: asd install <pkg_name>\n");
	printf("To remote pkg type: asd remote <pkg_name>\n");
	printf("To list all pkgs type: asd list\n");
	printf("To get info pkgs type: asd info <pkg_name>\n");
}

static int cmd_install(const char *name) {
    if (pkg_is_installed(name)) return 0;
    if (pkg_fetch_repo() != 0) return 1;

    char path[2048];
    if (pkg_find_path(name, path, sizeof(path)) != 0) return 1;

    Package p = {0};
    strncpy(p.name, name, sizeof(p.name) - 1);

    char buf[2048];

    snprintf(buf, sizeof(buf), "%s/info", path);
    if (pkg_parse_info(buf, &p) != 0) return 1;

    snprintf(buf, sizeof(buf), "%s/deps", path);
    pkg_parse_deps(buf, &p);

    for (int i = 0; i < p.dep_count; i++)
        if (!pkg_is_installed(p.deps[i]))
            if (cmd_install(p.deps[i]) != 0) return 1;

    snprintf(buf, sizeof(buf), "%s/make.sh", path);
    if (!file_exists(buf)) return 1;

    pid_t pid = fork();
    if (pid == 0) {
        setenv("PKG_NAME", p.name, 1);
        setenv("PKG_VERSION", p.version, 1);
        setenv("PKG_CATEGORY", p.category, 1);
        setenv("ASD_PREFIX", "/usr", 1);

        char jobs[16];
        snprintf(jobs, sizeof(jobs), "%ld", sysconf(_SC_NPROCESSORS_ONLN));
        setenv("ASD_JOBS", jobs, 1);

        execl("/bin/sh", "sh", buf, NULL);
        _exit(127);
    }

    if (pid < 0) return 1;

    int status;
    if (waitpid(pid, &status, 0) < 0) return 1;
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) return 1;

    pkg_mark_installed(&p);
    return 0;
}

static int cmd_remove(const char *name) {
    if (!pkg_is_installed(name)) return 1;
    return pkg_mark_removed(name);
}

static int cmd_list(void) {
    DIR *d = opendir(g_cfg.db_dir);
    if (!d) return 1;

    struct dirent *e;
    while ((e = readdir(d)))
        if (e->d_name[0] != '.')
            puts(e->d_name);

    closedir(d);
    return 0;
}

static int cmd_info(const char *name) {
    if (!pkg_is_installed(name)) return 1;

    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", g_cfg.db_dir, name);

    FILE *f = fopen(path, "r");
    if (!f) return 1;

    char buf[1024];
    while (fgets(buf, sizeof(buf), f))
        fputs(buf, stdout);

    fclose(f);
    return 0;
}

int main(int argc, char **argv) {
    config_load();

    if (argc < 2) return 1;

    if (!strcmp(argv[1], "install") && argc > 2)
        return cmd_install(argv[2]);

    if (!strcmp(argv[1], "remove") && argc > 2)
        return cmd_remove(argv[2]);

    if (!strcmp(argv[1], "list"))
        return cmd_list();

	if (!strcmp(argv[1], "help"))
		return cmd_help();
	
    if (!strcmp(argv[1], "info") && argc > 2)
        return cmd_info(argv[2]);

    return 1;
}
