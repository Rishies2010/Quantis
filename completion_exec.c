#include "quantis.h"

int is_executable(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return S_ISREG(st.st_mode) && (st.st_mode & S_IXUSR);
    }
    return 0;
}

int find_executables_in_path(const char *prefix,
                             char **completions,
                             int max_comp) {
    int count = 0;
    char *path_env = getenv("PATH");
    if (!path_env) return 0;

    char *path_copy = strdup(path_env);
    char *dir = strtok(path_copy, ":");
    size_t prefix_len = strlen(prefix);

    while (dir && count < max_comp) {
        DIR *d = opendir(dir);
        if (!d) {
            dir = strtok(NULL, ":");
            continue;
        }

        struct dirent *entry;
        while ((entry = readdir(d)) && count < max_comp) {
            if (strncmp(entry->d_name, prefix, prefix_len) == 0) {
                char full_path[PATH_BUF];
                snprintf(full_path, sizeof(full_path), "%s/%s",
                         dir, entry->d_name);

                if (is_executable(full_path)) {
                    int already_added = 0;
                    for (int i = 0; i < count; i++) {
                        if (strcmp(completions[i], entry->d_name) == 0) {
                            already_added = 1;
                            break;
                        }
                    }
                    if (!already_added) {
                        completions[count++] = strdup(entry->d_name);
                    }
                }
            }
        }
        closedir(d);
        dir = strtok(NULL, ":");
    }

    free(path_copy);
    return count;
}

