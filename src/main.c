#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#define SERVER_URL "https://raw.githubusercontent.com/Loxsete/plpkg-ports/main"
#define DOWNLOAD_BASE "/tmp/plpkg-cache"
#define UNPACK_BASE "/tmp/plpkg-unpack"
#define INSTALLED_DB "/etc/plpkgbase.bd"

void build_package(const char *category, const char *name);

int file_exists(const char *path) {
    struct stat sb;
    return (stat(path, &sb) == 0 && S_ISREG(sb.st_mode));
}

int directory_exists(const char *path) {
    struct stat sb;
    return (stat(path, &sb) == 0 && S_ISDIR(sb.st_mode));
}

int is_package_installed(const char *name)
{
    FILE *file = fopen(INSTALLED_DB, "r");
    if (!file) {
        return 0;
    }

    char line[128];
    int found = 0;

    while (fgets(line, sizeof(line), file))
    {
        line[strcspn(line, "\r\n")] = '\0';
        if (strcmp(line, name) == 0)
        {
            found = 1;
            break;
        }
    }
    fclose(file);
    return found;
}

void add_package_to_db(const char *name) 
{
    FILE *file = fopen(INSTALLED_DB, "a");
    if (!file) {
        printf("Error: Could not open installed database for writing\n");
        return;
    }
    fprintf(file, "%s\n", name);
    fclose(file);
}

int fetch_package(const char *category, const char *name, char *out_path, size_t max_len)
{
    snprintf(out_path, max_len, "%s/%s/%s.plpkg", DOWNLOAD_BASE, category, name);
    
    if (file_exists(out_path)) {
        return 1;
    }

    char mkdir_cmd[512];
    snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p %s/%s", DOWNLOAD_BASE, category);
    system(mkdir_cmd);

    printf("Downloading %s/%s.plpkg from server...\n", category, name);
    char wget_cmd[1024];
    snprintf(wget_cmd, sizeof(wget_cmd), "wget -qO %s \"%s/%s/%s.plpkg\"", out_path, SERVER_URL, category, name);

    if (system(wget_cmd) != 0) {
        remove(out_path);
        return 0;
    }
    return 1;
}

int find_and_fetch_dependency(const char *name, char *out_path, size_t max_len, char *found_category, size_t cat_len)
{
    printf("Searching server for dependency: %s...\n", name);

    char local_ports_path[512];
    snprintf(local_ports_path, sizeof(local_ports_path), "%s", DOWNLOAD_BASE);
    mkdir(local_ports_path, 0755);

    DIR *dir = opendir(local_ports_path);
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            char cat_path[512];
            snprintf(cat_path, sizeof(cat_path), "%s/%s", local_ports_path, entry->d_name);
            if (directory_exists(cat_path)) {
                char test_url[1024];
                snprintf(test_url, sizeof(test_url), "wget --spider -q \"%s/%s/%s.plpkg\"", SERVER_URL, entry->d_name, name);
                
                if (system(test_url) == 0) {
                    strncpy(found_category, entry->d_name, cat_len - 1);
                    found_category[cat_len - 1] = '\0';
                    closedir(dir);
                    return fetch_package(entry->d_name, name, out_path, max_len);
                }
            }
        }
        closedir(dir);
    }

    const char *fallback_categories[] = {"dev-lang", "dev-libs", "net-misc", "sys-libs", NULL};
    for (int i = 0; fallback_categories[i] != NULL; i++) {
        char test_url[1024];
        snprintf(test_url, sizeof(test_url), "wget --spider -q \"%s/%s/%s.plpkg\"", SERVER_URL, fallback_categories[i], name);
        
        if (system(test_url) == 0) {
            strncpy(found_category, fallback_categories[i], cat_len - 1);
            found_category[cat_len - 1] = '\0';
            return fetch_package(fallback_categories[i], name, out_path, max_len);
        }
    }

    return 0;
}

void resolve_and_build_dependencies(const char *unpack_dir, const char *name)
{
    char deps_path[512];
    snprintf(deps_path, sizeof(deps_path), "%s/deps", unpack_dir);
    
    FILE *file = fopen(deps_path, "r");
    if (!file) {
        return;
    }

    printf("Checking dependencies for %s... \n", name);
    char dep_name[128];
    while (fgets(dep_name, sizeof(dep_name), file))
    {
        dep_name[strcspn(dep_name, "\r\n")] = '\0';
        if (strlen(dep_name) == 0) continue;

        char dep_pkg_path[512];
        char dep_category[128];

        if (find_and_fetch_dependency(dep_name, dep_pkg_path, sizeof(dep_pkg_path), dep_category, sizeof(dep_category))) {
            printf("Found dependency package on server. Building it first...\n");
            build_package(dep_category, dep_name);
        } else {
            printf("Warning: Dependency package '%s' not found on server!\n", dep_name);
        }
    }
    fclose(file);
}

void build_package(const char *category, const char *name)
{   
    if (is_package_installed(name)) {
        printf("Package %s is already installed. Skipping.\n", name);
        return;
    }

    char pkg_path[512];
    if (!fetch_package(category, name, pkg_path, sizeof(pkg_path))) {
        printf("Error: Failed to fetch package %s/%s from server\n", category, name);
        return;
    }

    char unpack_dir[512];
    snprintf(unpack_dir, sizeof(unpack_dir), "%s/%s", UNPACK_BASE, name);

    char clean_cmd[1024];
    snprintf(clean_cmd, sizeof(clean_cmd), "rm -rf %s && mkdir -p %s", unpack_dir, unpack_dir);
    system(clean_cmd);

    printf("Unpacking package %s to %s...\n", pkg_path, unpack_dir);
    char unpack_cmd[1024];
    snprintf(unpack_cmd, sizeof(unpack_cmd), "tar -xzf %s -C %s", pkg_path, unpack_dir);
    
    if (system(unpack_cmd) != 0) {
        printf("Error: Failed to unpack %s. Is it a valid archive?\n", pkg_path);
        return;
    }

    resolve_and_build_dependencies(unpack_dir, name);

    char info_path[512];
    snprintf(info_path, sizeof(info_path), "%s/info", unpack_dir);

    FILE *file = fopen(info_path, "r");
    char version[128] = "unknown";
    if (file) {
        char line[256];
        while (fgets(line, sizeof(line), file)) {
            if (strncmp(line, "version=", 8) == 0) {
                line[strcspn(line, "\r\n")] = '\0';
                strncpy(version, line + 8, sizeof(version) - 1);
                break;
            }
        }
        fclose(file);
    }

    printf("Starting build for %s (version %s)\n", name, version);

    setenv("PKG_NAME", name, 1);
    setenv("PKG_VERSION", version, 1);
    setenv("ASD_PREFIX", "/usr/local", 1); 
    setenv("ASD_JOBS", "4", 1);

    char script_path[512];
    snprintf(script_path, sizeof(script_path), "%s/make.sh", unpack_dir);

    if (access(script_path, F_OK) != 0) {
        printf("Error: Script of build %s not found\n", script_path);
        return;
    }

    char command[1024];
    snprintf(command, sizeof(command), "bash %s", script_path);

    int status = system(command);

    if (status == 0) {
        printf("Package %s successfully built and installed\n", name);
        add_package_to_db(name);
    } else {
        printf("Package %s build error\n", name);
    }

    snprintf(clean_cmd, sizeof(clean_cmd), "rm -rf %s", unpack_dir);
    system(clean_cmd);
}

int main(int argc, char *argv[])
{
    printf("hello world\n");

    if (argc < 3) 
    {
        printf("\nUsage: %s <category> <package_name>\n", argv[0]);
        printf("Example: %s net-misc curl\n", argv[0]);
        return 1;
    }

    build_package(argv[1], argv[2]);
    return 0;
}
