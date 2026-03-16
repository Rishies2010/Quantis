#include "quantis.h"

char *read_command_line(int prompt_len) {
    (void)prompt_len;
    char *line_buffer = calloc(MAX_LINE, 1);
    if (!line_buffer) {
        perror("calloc");
        return NULL;
    }
    int len = 0;
    history_current = history_count;

    while (1) {
        char c;
        if (read(STDIN_FILENO, &c, 1) <= 0) {
            free(line_buffer);
            return NULL;
        }

        if (c == '\t') {
            int result = handle_tab_completion(line_buffer, &len);
            if (result == 2) {
                char *prompt = build_prompt();
                printf("%s%s", prompt, line_buffer);
                fflush(stdout);
                free(prompt);
            }
            continue;
        }

        if (c == '\r' || c == '\n') {
            line_buffer[len] = '\0';
            write(STDOUT_FILENO, "\n", 1);
            break;
        }

        if (c == 127 || c == '\b') {
            if (len > 0) {
                len--;
                line_buffer[len] = '\0';
                write(STDOUT_FILENO, "\b \b", 3);
            }
            continue;
        }

        if (c == 27) {
            char seq[3];
            if (read(STDIN_FILENO, &seq[0], 1) != 1) continue;
            if (read(STDIN_FILENO, &seq[1], 1) != 1) continue;

            if (seq[0] == '[' &&
                (seq[1] == 'A' || seq[1] == 'B')) {
                int new_history_index = history_current;

                if (seq[1] == 'A') {
                    if (new_history_index > 0) new_history_index--;
                } else if (seq[1] == 'B') {
                    if (new_history_index < history_count)
                        new_history_index++;
                }

                if (new_history_index != history_current) {
                    history_current = new_history_index;
                    printf("\r\033[K");
                    fflush(stdout);

                    char *history_line = NULL;
                    if (history_current < history_count) {
                        history_line = history[history_current];
                    }

                    if (history_line) {
                        len = snprintf(line_buffer, MAX_LINE,
                                       "%s", history_line);
                    } else {
                        len = 0;
                        line_buffer[0] = '\0';
                    }

                    printf("  %s", line_buffer);
                    fflush(stdout);
                }
            }
            continue;
        }

        if (len < MAX_LINE - 1) {
            line_buffer[len++] = c;
            line_buffer[len] = '\0';
            write(STDOUT_FILENO, &c, 1);
        }
    }

    return line_buffer;
}

