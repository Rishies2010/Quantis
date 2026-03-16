#include "quantis.h"

char *expand_aliases(const char *input_line) {
    char *temp_line = strdup(input_line);
    if (!temp_line) return strdup(input_line);

    char *first_word = strtok(temp_line, " \t");

    if (!first_word) {
        free(temp_line);
        return strdup(input_line);
    }

    for (int i = 0; i < alias_count; i++) {
        if (strcmp(first_word, aliases[i].name) == 0) {
            const char *alias_value = aliases[i].value;
            size_t len = strlen(alias_value);

            size_t first_word_len = strlen(first_word);
            const char *rest_of_line = input_line + first_word_len;

            while (*rest_of_line == ' ' || *rest_of_line == '\t') {
                rest_of_line++;
            }

            size_t rest_len = strlen(rest_of_line);
            size_t expanded_len =
                len + (rest_len > 0 ? 1 : 0) + rest_len + 1;

            char *expanded = malloc(expanded_len);
            if (!expanded) {
                perror("malloc for expanded alias");
                free(temp_line);
                return strdup(input_line);
            }

            strcpy(expanded, alias_value);

            if (rest_len > 0) {
                strcat(expanded, " ");
                strcat(expanded, rest_of_line);
            }

            free(temp_line);
            return expanded;
        }
    }

    free(temp_line);
    return strdup(input_line);
}

void load_aliases(const char *rc_file) {
    FILE *f = fopen(rc_file, "r");
    if (!f) return;

    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0;
        char *trimmed = line;
        while (*trimmed == ' ' || *trimmed == '\t') trimmed++;

        if (strlen(trimmed) == 0 || trimmed[0] == '#') continue;

        if (strncmp(trimmed, "alias ", 6) == 0) {
            char *alias_def = trimmed + 6;
            char *colon = strchr(alias_def, ':');
            if (!colon) continue;

            *colon = '\0';
            char *name = alias_def;

            char *name_end = name + strlen(name) - 1;
            while (name_end >= name &&
                   (*name_end == ' ' || *name_end == '\t'))
                name_end--;
            *(name_end + 1) = '\0';

            char *value = extract_alias_value(colon + 1);

            if (strlen(name) > 0 && value) {
                add_alias(name, value);
            }
            if (value) free(value);
        }
    }
    fclose(f);
}
