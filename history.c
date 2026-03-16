#include "quantis.h"

void add_to_history(const char *line) {
    char *temp = strdup(line);
    char *trimmed = temp;
    while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
    if (*trimmed == '\0') {
        free(temp);
        return;
    }

    if (history_count > 0 &&
        strcmp(history[history_count - 1], line) == 0) {
        free(temp);
        return;
    }
    free(temp);

    if (history_count < MAX_HISTORY) {
        history[history_count++] = strdup(line);
    } else {
        free(history[0]);
        for (int i = 1; i < MAX_HISTORY; i++) {
            history[i - 1] = history[i];
        }
        history[MAX_HISTORY - 1] = strdup(line);
    }
    history_current = history_count;
}

void load_history(const char *hist_file) {
    FILE *f = fopen(hist_file, "r");
    if (!f) return;

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) > 0 && line[0] != '#') {
            if (history_count < MAX_HISTORY) {
                history[history_count++] = strdup(line);
            }
        }
    }
    fclose(f);
    history_current = history_count;
}

void save_history(const char *hist_file) {
    FILE *f = fopen(hist_file, "w");
    if (!f) {
        perror(" Quantis: Error while attempting to save history.\n");
        return;
    }
    for (int i = 0; i < history_count; i++) {
        fprintf(f, "%s\n", history[i]);
    }
    fclose(f);
}

