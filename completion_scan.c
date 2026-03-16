#include "quantis.h"

int find_file_completions(const char *prefix,
                          char **completions,
                          int max_comp) {
    int count = 0;
    char *expanded = expand_tilde(prefix);
    char *last_slash = strrchr(expanded, '/');

    char *dir_path = NULL;
    char *file_prefix = NULL;
    int has_slash = (last_slash != NULL);

    if (last_slash) {
        *last_slash = '\0';
        dir_path = (*expanded == '\0') ? "/" : expanded;
        file_prefix = last_slash + 1;
    } else {
        dir_path = ".";
        file_prefix = expanded;
    }

    DIR *d = opendir(dir_path);
    if (!d) {
        free(expanded);
        return 0;
    }

    size_t prefix_len = strlen(file_prefix);
    struct dirent *entry;

    while ((entry = readdir(d)) && count < max_comp) {
        if (entry->d_name[0] == '.' && file_prefix[0] != '.')
            continue;

        if (strncmp(entry->d_name, file_prefix, prefix_len) == 0) {
            if (has_slash) {
                size_t total_len = strlen(prefix) - prefix_len
                                 + strlen(entry->d_name) + 1;
                char *full = malloc(total_len);
                snprintf(full, total_len, "%.*s%s",
                         (int)(strlen(prefix) - prefix_len),
                         prefix, entry->d_name);
                completions[count++] = full;
            } else {
                completions[count++] = strdup(entry->d_name);
            }
        }
    }

    closedir(d);
    free(expanded);
    return count;
}

int compare_strings(const void *a, const void *b) {
    return strcmp(*(const char *const *)a, *(const char *const *)b);
}
