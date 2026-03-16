#include "quantis.h"

int quantis_main(int argc, char *argv[]) {
    signal(SIGINT, sigint_handler);

    if (argc > 1) {
        if (strcmp(argv[1], "--version") == 0 ||
            strcmp(argv[1], "-v") == 0) {
            print_version();
            return 0;
        } else if (strcmp(argv[1], "--help") == 0 ||
                   strcmp(argv[1], "-h") == 0) {
            printf(FG_CYAN "Quantis" COL_RESET "\n");
            printf("Version  %s\n\n", _VER);
            printf("Usage  Quantis [OPTIONS]\n\n");
            printf("Options \n");
            printf("  --help, -h      Show this help message\n");
            printf("  --version, -v   Show version information\n\n");
            printf("Builtin commands \n");
            printf("  cd              Change directory\n");
            printf("  exit            Exit the shell\n");
            printf("  clear           Clear the screen\n");
            printf("  help            Show builtin commands\n");
            printf("  alias           Create or list aliases\n");
            printf("  unalias         Remove an alias\n\n");
            return 0;
        } else {
            print_unknown_option(argv[1]);
            return 1;
        }
    }

    return run_shell();
}

