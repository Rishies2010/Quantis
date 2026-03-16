#include "quantis.h"

int handle_builtin(char **argv, char *rc_file, char *hist_file) {
    (void)hist_file;
    if (!argv[0]) return 1;
    if (!strcmp(argv[0], "exit")) {
        run = 0;
        return 1;
    }
    if (!strcmp(argv[0], "cd")) {
        const char *d = argv[1] ? argv[1] : getenv("HOME");
        char *expanded = expand_tilde(d);
        if (chdir(expanded) != 0)
            perror(" Quantis: cd");
        free(expanded);
        return 1;
    }
    if (!strcmp(argv[0], "clear")) {
        printf("\033[H\033[2J");
        return 1;
    }
    if (!strcmp(argv[0], "help")) {
        print_help();
        return 1;
    }
    if (!strcmp(argv[0], "alias")) {
        if (!argv[1]) {
            for (int i = 0; i < alias_count; i++) {
                printf("Alias %i : %s  %s\n",
                       (i + 1),
                       aliases[i].name,
                       aliases[i].value);
            }
        } else {
            size_t total_len = 0;
            int i = 1;
            while (argv[i]) {
                total_len += strlen(argv[i]) + 1;
                i++;
            }

            char *alias_def = calloc(total_len, 1);
            if (!alias_def) {
                perror("calloc");
                return 1;
            }

            i = 1;
            while (argv[i]) {
                strcat(alias_def, argv[i]);
                if (argv[i + 1]) strcat(alias_def, " ");
                i++;
            }

            char *colon = strchr(alias_def, ':');
            if (!colon) {
                fprintf(stderr,
                        " Quantis: alias: "
                        "Usage: alias name:{alias name}\n");
                free(alias_def);
                return 1;
            }

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
                save_aliases(rc_file);
            } else {
                fprintf(stderr,
                        " Quantis: alias: "
                        "Invalid value extraction.\n");
            }

            if (value) free(value);
            free(alias_def);
        }
        return 1;
    }
    if (!strcmp(argv[0], "unalias")) {
        if (!argv[1]) {
            fprintf(stderr,
                    " Quantis: unalias: Usage: unalias name\n");
        } else {
            remove_alias(argv[1]);
            save_aliases(rc_file);
        }
        return 1;
    }
    return 0;
}
