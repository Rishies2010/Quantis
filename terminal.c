#include "quantis.h"

void reset_terminal(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &saved_tattr);
}

void set_raw_mode(void) {
    struct termios tattr;
    tcgetattr(STDIN_FILENO, &saved_tattr);
    atexit(reset_terminal);

    tattr = saved_tattr;
    tattr.c_lflag &= ~(ICANON | ECHO);
    tattr.c_cc[VMIN] = 1;
    tattr.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &tattr);
}

void sigint_handler(int s) {
    (void)s;
    if (child_pid)
        kill(child_pid, SIGINT);
    else
        write(STDOUT_FILENO, "\n", 1);
}

