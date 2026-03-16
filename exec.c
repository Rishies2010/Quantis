#include "quantis.h"

void execute_command(char **argv, int bg) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }
    if (pid == 0) {
        signal(SIGINT, SIG_DFL);
        execvp(argv[0], argv);
        fprintf(stderr, " Quantis: %s: %s\n",
                argv[0], strerror(errno));
        _exit(127);
    }
    if (bg) {
        printf("[%d] %d\n", getpid(), pid);
    } else {
        child_pid = pid;
        waitpid(pid, NULL, 0);
        child_pid = 0;
    }
}

