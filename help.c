#include "quantis.h"

void print_version(void) {
    printf("\n" FG_CYAN "Quantis " COL_RESET "version " _VER "\n\n");

    struct passwd *pw = getpwuid(getuid());
    printf("Usage  Quantis [OPTIONS]\n\n");
    printf("Options \n");
    printf("  --help, -h      Show this help message\n");
    printf("  --version, -v   Show version information\n\n");
    printf("User  %s\n", pw ? pw->pw_name : "unknown");
    printf("UID  %d\n\n", getuid());
}

void print_help(void) {
    printf("\nWelcome to " FG_CYAN "Quantis" COL_RESET ".\n");
    printf("Version  %s\n\n", _VER);
    printf("Usage  Quantis [OPTIONS]\n\n");
    printf("Options \n");
    printf("  --help, -h      Show this help message\n");
    printf("  --version, -v   Show version information\n\n");
    printf("Builtin commands:\n");
    printf("  cd              Change directory\n");
    printf("  exit            Exit the shell\n");
    printf("  clear           Clear the screen\n");
    printf("  help            Show builtin commands\n");
    printf("  alias           Create or list aliases\n");
    printf("  unalias         Remove an alias\n\n");
}

void print_unknown_option(const char *opt) {
    fprintf(stderr, "Quantis: unknown option: %s\n", opt);
    fprintf(stderr,
            "Try 'Quantis --help' for more information.\n");
}

