#ifndef QUANTIS_H
#define QUANTIS_H

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>

#define MAX_ARGS 128
#define PROMPT_BUF 512
#define MAX_ALIASES 50
#define MAX_HISTORY 1000
#define MAX_LINE 1024
#define PATH_BUF 4096
#define MAX_COMPLETIONS 256
#define _VER "1.0_dev"
#define COL_RESET "\033[0m"
#define FG_BLACK "\033[30m"
#define FG_PURPLE "\033[38;2;168;162;238m"
#define FG_CYAN "\033[38;2;100;220;240m"
#define FG_GRAY "\033[38;5;240m"
#define BG_BLACK "\033[40m"
#define BG_PURPLE "\033[48;2;168;162;238m"
#define BG_CYAN "\033[48;2;100;220;240m"

typedef struct {
    char *name;
    char *value;
} alias_t;

extern volatile pid_t child_pid;
extern int run;
extern struct termios saved_tattr;
extern alias_t aliases[MAX_ALIASES];
extern int alias_count;
extern char *history[MAX_HISTORY];
extern int history_count;
extern int history_current;

/* terminal */
void reset_terminal(void);
void set_raw_mode(void);
void sigint_handler(int s);

/* path helpers */
char *expand_tilde(const char *path);
char *get_program_directory(void);

/* prompt and input */
char *build_prompt(void);
int handle_tab_completion(char *line_buffer, int *len);
char *read_command_line(int prompt_len);

/* history */
void add_to_history(const char *line);
void load_history(const char *hist_file);
void save_history(const char *hist_file);

/* aliases */
char *extract_alias_value(const char *input);
void add_alias(const char *name, const char *value);
void remove_alias(const char *name);
void save_aliases(const char *rc_file);
char *expand_aliases(const char *input_line);
void load_aliases(const char *rc_file);

/* completion helpers */
int is_executable(const char *path);
int find_executables_in_path(const char *prefix, char **completions, int max_comp);
int find_file_completions(const char *prefix, char **completions, int max_comp);
int compare_strings(const void *a, const void *b);

/* parsing and execution */
int parse_line(char *line, char **argv, int *bg);
int handle_builtin(char **argv, char *rc_file, char *hist_file);
void execute_command(char **argv, int bg);

/* shell lifecycle */
void ensure_file(const char *path, const char *def);
int run_shell(void);
void print_version(void);
void print_help(void);
void print_unknown_option(const char *opt);
int quantis_main(int argc, char *argv[]);

#endif
