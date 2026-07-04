#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#define REPO_URL "https://github.com/Loxsete/asd-ports/archive/refs/heads/main.tar.gz"
#define PORTS_PATH "/tmp/asd-ports" // need to replace into /tmp/ or something else
#define INSTALLED_DB "/etc/asdbase.bd"


void build(const char *category, const char *name); // for recursion

int directory_exists(const char *path) {
    struct stat sb;
    return (stat(path, &sb) == 0 && S_ISDIR(sb.st_mode));
}

void update_or_clone_ports(void)
{
    if (directory_exists(PORTS_PATH))
    {
        printf("Ports tree already in '%s'. Skiping cloning\n", PORTS_PATH);
        return;
    }

    printf("Download ports tree from %s...\n", REPO_URL);

    char command[2048];

    snprintf(command, sizeof(command),
        "mkdir -p /tmp && "
        "wget -qO /tmp/asd-ports.tar.gz \"%s\" && "
        "tar -xzf /tmp/asd-ports.tar.gz -C /tmp && "
        "mv /tmp/asd-ports-main \"%s\" && "
        "rm -f /tmp/asd-ports.tar.gz",
        REPO_URL,
        PORTS_PATH);

    int status = system(command);

    if (status == 0)
    {
        printf("Ports tree sucessfully sync in '%s'!\n", PORTS_PATH);
    }
    else
    {
        printf("Error downloading ports\n");
    }
}

int get_version_from_info(const char *category, const char *name, char *version_out, size_t max_len)
{
    char info_path[512];
    snprintf(info_path, sizeof(info_path), "%s/%s/%s/info", PORTS_PATH, category, name);

    FILE *file = fopen(info_path, "r");
    if (!file) {
        return 0;
    }

    char line[256];
    int found = 0;

    while (fgets(line, sizeof(line), file)) 
    {
        if (strncmp(line, "version=", 8) == 0) 
        {
            line[strcspn(line, "\r\n")] = '\0';
            strncpy(version_out, line + 8, max_len - 1);

            version_out[max_len - 1] = '\0';
            
            found = 1;
            
            break;
        }
    }
    fclose(file);
    return found;
}

int find_category_for_package(const char *name, char *category_out, size_t max_len)
{
    DIR *dir = opendir(PORTS_PATH);
    if (!dir) {
        return 0;
    }

    struct dirent *entry;
    char test_path[512];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, ".git") == 0) {
            continue;
        }

        snprintf(test_path, sizeof(test_path), "%s/%s", PORTS_PATH, entry->d_name);
        if (directory_exists(test_path)) {
            snprintf(test_path, sizeof(test_path), "%s/%s/%s", PORTS_PATH, entry->d_name, name);
            if (directory_exists(test_path)) {
                strncpy(category_out, entry->d_name, max_len - 1);
                category_out[max_len - 1] = '\0';
                closedir(dir);
                return 1;
            }
        }
    }

    closedir(dir);
    return 0;
}

void resolve_and_build_dependencies(const char *category, const char *name)
{
    char deps_path[512];
    snprintf(deps_path, sizeof(deps_path), "%s/%s/%s/deps", PORTS_PATH, category, name);
    
    FILE *file = fopen(deps_path, "r");
    if (!file)
    {
        return;
    }
    printf("Checking dependiencies for %s... \n", name);
    char dep_name[128];
    while (fgets(dep_name, sizeof(dep_name), file))
    {
        dep_name[strcspn(dep_name, "\r\n")] = '\0';
        if (strlen(dep_name) == 0) continue;

        char dep_category[128];
        if (find_category_for_package(dep_name, dep_category, sizeof(dep_category))) {
            printf("[asd] Found dependency: %s/%s. Building it first...\n", dep_category, dep_name);
            
            // recursion here !
            build(dep_category, dep_name);
        } else {
            printf("[asd] Warning: Dependency '%s' required by '%s' not found in ports tree!\n", dep_name, name);
        }
    }
    fclose(file);
}

int is_package_installed(const char *name)
{
    FILE *file = fopen(INSTALLED_DB, "r");
    // if file not exists, nothing is installed
    if (!file)
    {
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


void build(const char *category, const char *name)
{   
    if (is_package_installed(name)) {
        printf("Package %s is already installed. Skipping build.\n", name);
        return;
    }

    resolve_and_build_dependencies(category, name);

    char version[128];

    if (!get_version_from_info(category, name, version, sizeof(version)))
    {
        printf("Error: Could not detect version from 'info' file for %s/%s\n", category, name);
        return;
    }

    printf("Automatically detected version is %s\n", version);

    printf("Starting build for %s/%s (version %s)\n", category, name, version);

    setenv("PKG_NAME", name, 1);
    setenv("PKG_VERSION", version, 1);
    setenv("ASD_PREFIX", "/usr/local", 1); 
    setenv("ASD_JOBS", "4", 1);

    char script_path[512];
    snprintf(script_path, sizeof(script_path), "%s/%s/%s/make.sh", PORTS_PATH, category, name);

    // has make.sh be?
    if (access(script_path, F_OK) != 0)
    {
        printf("Error: Script of build %s not found\n", script_path);
        return;
    }

    // forming a command for running bash-script
    char command[1024];
    snprintf(command, sizeof(command), "bash %s", script_path);

    int status = system(command);

    if (status == 0)
    {
        printf("Package %s successfully built and installed\n", name);
        add_package_to_db(name);
    } else {
        printf("Package %s build error\n", name);
    }
}

int main(int argc, char *argv[])
{
    printf("hello world\n");
    
    update_or_clone_ports();

    if (argc < 3) 
    {
        printf("\nUsage: %s <category> <package_name>\n", argv[0]);
        printf("Example: %s net-misc curl\n", argv[0]);
        return 1;
    }

    build(argv[1], argv[2]);
    // netx logic
    return 0;
}
