#include "quantis.h"

char *extract_alias_value(const char *input) {
    char *open_brace = strchr(input, '{');
    char *close_brace = strrchr(input, '}');

    if (!open_brace || !close_brace || open_brace >= close_brace)
        return NULL;

    size_t len = (size_t)(close_brace - open_brace - 1);
    char *value = malloc(len + 1);
    if (value) {
        strncpy(value, open_brace + 1, len);
        value[len] = '\0';
    }
    return value;
}

void add_alias(const char *name, const char *value) {
    if (alias_count >= MAX_ALIASES) {
        fprintf(stderr, " Quantis: Too many aliases.\n");
        return;
    }
    if (strcmp(name, "alias") == 0) {
        fprintf(stderr, " Quantis: Cannot make an alias for alias.\n");
        return;
    } else if (strcmp(name, "unalias") == 0) {
        fprintf(stderr, " Quantis: Cannot make an alias for unalias.\n");
        return;
    } else if (strcmp(name, "cd") == 0) {
        fprintf(stderr, " Quantis: Cannot make an alias for cd.\n");
        return;
    }

    char *value_copy = strdup(value);

    for (int i = 0; i < alias_count; i++) {
        if (strcmp(aliases[i].name, name) == 0) {
            free(aliases[i].value);
            aliases[i].value = value_copy;
            return;
        }
    }

    aliases[alias_count].name = strdup(name);
    aliases[alias_count].value = value_copy;
    alias_count++;
}

void remove_alias(const char *name) {
    for (int i = 0; i < alias_count; i++) {
        if (strcmp(aliases[i].name, name) == 0) {
            free(aliases[i].name);
            free(aliases[i].value);
            for (int j = i; j < alias_count - 1; j++) {
                aliases[j] = aliases[j + 1];
            }
            alias_count--;
            return;
        }
    }
}

