#include "quantis.h"

char *expand_tilde(const char *path) {
    if (!path) {
        /* Fallback when no path/HOME is available. */
        return strdup(".");
    }
    if (path[0] != '~') {
        return strdup(path);
    }

    const char *home = getenv("HOME");
    if (!home) {
        struct passwd *pw = getpwuid(getuid());
        home = pw ? pw->pw_dir : ".";
    }

    size_t home_len = strlen(home);
    size_t path_len = strlen(path);
    char *expanded = malloc(home_len + path_len);

    if (expanded) {
        strcpy(expanded, home);
        strcat(expanded, path + 1);
    }

    return expanded;
}

char *get_program_directory(void) {
    char path[PATH_BUF];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);

    if (len != -1) {
        path[len] = '\0';
        char *last_slash = strrchr(path, '/');
        if (last_slash) *last_slash = '\0';
    } else {
        if (!getcwd(path, sizeof(path))) {
            return strdup(".");
        }
    }
    return strdup(path);
}
