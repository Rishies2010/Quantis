#include "quantis.h"

int parse_line(char *line, char **argv, int *bg) {
    int argc = 0;
    char *t;
    *bg = 0;

    t = strtok(line, " \t");
    while (t && argc < MAX_ARGS - 1) {
        if (strcmp(t, "&") == 0) {
            *bg = 1;
            break;
        } else {
            argv[argc++] = t;
        }
        t = strtok(NULL, " \t");
    }

    argv[argc] = NULL;
    return argc;
}

