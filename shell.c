#include "quantis.h"

int run_shell(void) {
    set_raw_mode();

    char *input = NULL;
    char *prompt = NULL;
    char *args[MAX_ARGS];
    int bg;

    char *prog_dir = get_program_directory();
    char *rc = malloc(strlen(prog_dir) + 20);
    char *hist = malloc(strlen(prog_dir) + 20);
    if (!rc || !hist) {
        perror("malloc for config paths");
        exit(EXIT_FAILURE);
    }

    sprintf(rc, "%s/.qnrc", prog_dir);
    sprintf(hist, "%s/.qnhistory", prog_dir);

    const char *default_rc = "";
    ensure_file(rc, default_rc);
    ensure_file(hist, "# .qnhistory\n\n");

    if (!getenv("TERM")) setenv("TERM", "xterm-kitty", 1);
    if (!getenv("COLORTERM"))
        setenv("COLORTERM", "truecolor", 1);

    printf("\033c");
    load_aliases(rc);
    load_history(hist);

    while (run) {
        prompt = build_prompt();
        printf("%s", prompt);
        fflush(stdout);

        int prompt_len =
            8 + (int)strlen(getpwuid(getuid())->pw_name) +
            (int)strlen(getcwd(NULL, 0));
        input = read_command_line(prompt_len);

        free(prompt);

        if (!input) break;

        if (!*input) {
            free(input);
            continue;
        }

        add_to_history(input);

        char *expanded = expand_aliases(input);
        free(input);

        if (!expanded) continue;

        char *dup = strdup(expanded);
        if (!dup) {
            perror("strdup for parsing");
            free(expanded);
            continue;
        }

        int argc_parsed = parse_line(dup, args, &bg);

        if (argc_parsed == 0) {
            free(dup);
            free(expanded);
            continue;
        }

        if (!handle_builtin(args, rc, hist))
            execute_command(args, bg);

        free(dup);
        free(expanded);
    }

    save_history(hist);
    save_aliases(rc);

    for (int i = 0; i < alias_count; i++) {
        free(aliases[i].name);
        free(aliases[i].value);
    }
    for (int i = 0; i < history_count; i++) {
        free(history[i]);
    }

    free(rc);
    free(hist);
    free(prog_dir);
    printf("\n Exiting Quantis...\n\n");
    return 0;
}

