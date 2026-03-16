#include "quantis.h"

volatile pid_t child_pid = 0;
int run = 1;
struct termios saved_tattr;

alias_t aliases[MAX_ALIASES];
int alias_count = 0;

char *history[MAX_HISTORY];
int history_count = 0;
int history_current = 0;

int main(int argc, char *argv[]) {
    return quantis_main(argc, argv);
}
