#include "quantis.h"

int handle_tab_completion(char *line_buffer, int *len) {
    if (*len == 0) return 0;

    char temp_line[MAX_LINE];
    strncpy(temp_line, line_buffer, *len);
    temp_line[*len] = '\0';

    char *last_space = strrchr(temp_line, ' ');
    char *word_start = last_space ? (last_space + 1) : temp_line;
    int word_pos = (int)(word_start - temp_line);

    char *completions[MAX_COMPLETIONS];
    int comp_count = 0;

    if (word_pos == 0) {
        comp_count = find_executables_in_path(
            word_start, completions, MAX_COMPLETIONS);
    } else {
        comp_count = find_file_completions(
            word_start, completions, MAX_COMPLETIONS);
    }

    if (comp_count == 0) {
        return 0;
    }

    qsort(completions, comp_count,
          sizeof(char *), compare_strings);

    if (comp_count == 1) {
        int old_word_len = (int)strlen(word_start);
        int new_word_len = (int)strlen(completions[0]);

        for (int i = 0; i < old_word_len; i++) {
            write(STDOUT_FILENO, "\b \b", 3);
        }

        strcpy(line_buffer + word_pos, completions[0]);
        *len = word_pos + new_word_len;
        line_buffer[*len] = '\0';

        write(STDOUT_FILENO,
              completions[0], new_word_len);

        free(completions[0]);
        return 1;
    }

    size_t common_len = strlen(completions[0]);
    for (int i = 1; i < comp_count; i++) {
        size_t j = 0;
        while (j < common_len &&
               completions[0][j] == completions[i][j]) {
            j++;
        }
        common_len = j;
    }

    if (common_len > strlen(word_start)) {
        int old_word_len = (int)strlen(word_start);

        for (int i = 0; i < old_word_len; i++) {
            write(STDOUT_FILENO, "\b \b", 3);
        }

        strncpy(line_buffer + word_pos,
                completions[0], common_len);
        *len = word_pos + (int)common_len;
        line_buffer[*len] = '\0';

        write(STDOUT_FILENO,
              line_buffer + word_pos, common_len);
    } else {
        printf("\n" FG_GRAY " " COL_RESET " ");
        for (int i = 0; i < comp_count; i++) {
            printf("%s  ", completions[i]);
        }
        printf("\n");
    }

    for (int i = 0; i < comp_count; i++) {
        free(completions[i]);
    }

    return (comp_count > 1 &&
            common_len <= strlen(word_start)) ? 2 : 1;
}

