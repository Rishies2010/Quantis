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

static volatile pid_t child_pid = 0;
static int run = 1;
static struct termios saved_tattr;

typedef struct {
    char *name;
    char *value;
} alias_t;

alias_t aliases[MAX_ALIASES];
int alias_count = 0;

char *history[MAX_HISTORY];
int history_count = 0;
int history_current = 0;

void reset_terminal() {
    tcsetattr(STDIN_FILENO, TCSANOW, &saved_tattr);
}

void set_raw_mode() {
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
    if (child_pid)
        kill(child_pid, SIGINT);
    else
        write(1, "\n", 1);
}

char *expand_tilde(const char *path) {
    if (path[0] != '~')
        return strdup(path);
    
    const char *home = getenv("HOME");
    if (!home) {
        struct passwd *pw = getpwuid(getuid());
        home = pw ? pw->pw_dir : ".";
    }
    
    size_t home_len = strlen(home);
    size_t path_len = strlen(path);
    char *expanded = malloc(home_len + path_len);
    
    if (expanded) {
        strcpy(expanded, home);
        strcat(expanded, path + 1);
    }
    
    return expanded;
}

char *build_prompt()
{
    char buf[PROMPT_BUF], cwd[PATH_MAX];
    struct passwd *pw = getpwuid(getuid());
    const char *user = pw ? pw->pw_name : "user";

    if (!getcwd(cwd, sizeof(cwd)))
        strcpy(cwd, "?");

    snprintf(buf, sizeof(buf),
             BG_BLACK FG_PURPLE "" COL_RESET
                 BG_PURPLE FG_BLACK " %s " COL_RESET
                     BG_BLACK FG_PURPLE "" COL_RESET
                                " ~\n"
                                "  " BG_BLACK FG_CYAN "" COL_RESET
                                    BG_CYAN FG_BLACK " %s " COL_RESET
                                        BG_BLACK FG_CYAN "" COL_RESET
                                "  ",
             user, cwd);
    return strdup(buf);
}

void strip_quotes(char **str) {
    char *s = *str;
    size_t len = strlen(s);
    if (len >= 2) {
        if ((s[0] == '"' && s[len-1] == '"') || (s[0] == '\'' && s[len-1] == '\'')) {
            s[len-1] = '\0';
            memmove(s, s+1, len-1);
        }
    }
}

int is_executable(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return S_ISREG(st.st_mode) && (st.st_mode & S_IXUSR);
    }
    return 0;
}

int find_executables_in_path(const char *prefix, char **completions, int max_comp) {
    int count = 0;
    char *path_env = getenv("PATH");
    if (!path_env) return 0;
    
    char *path_copy = strdup(path_env);
    char *dir = strtok(path_copy, ":");
    size_t prefix_len = strlen(prefix);
    
    while (dir && count < max_comp) {
        DIR *d = opendir(dir);
        if (!d) {
            dir = strtok(NULL, ":");
            continue;
        }
        
        struct dirent *entry;
        while ((entry = readdir(d)) && count < max_comp) {
            if (strncmp(entry->d_name, prefix, prefix_len) == 0) {
                char full_path[PATH_BUF];
                snprintf(full_path, sizeof(full_path), "%s/%s", dir, entry->d_name);
                
                if (is_executable(full_path)) {
                    int already_added = 0;
                    for (int i = 0; i < count; i++) {
                        if (strcmp(completions[i], entry->d_name) == 0) {
                            already_added = 1;
                            break;
                        }
                    }
                    if (!already_added) {
                        completions[count++] = strdup(entry->d_name);
                    }
                }
            }
        }
        closedir(d);
        dir = strtok(NULL, ":");
    }
    
    free(path_copy);
    return count;
}

int find_file_completions(const char *prefix, char **completions, int max_comp) {
    int count = 0;
    char *expanded = expand_tilde(prefix);
    char *last_slash = strrchr(expanded, '/');
    
    char *dir_path = NULL;
    char *file_prefix = NULL;
    int has_slash = (last_slash != NULL);
    
    if (last_slash) {
        *last_slash = '\0';
        dir_path = (*expanded == '\0') ? "/" : expanded;
        file_prefix = last_slash + 1;
    } else {
        dir_path = ".";
        file_prefix = expanded;
    }
    
    DIR *d = opendir(dir_path);
    if (!d) {
        free(expanded);
        return 0;
    }
    
    size_t prefix_len = strlen(file_prefix);
    struct dirent *entry;
    
    while ((entry = readdir(d)) && count < max_comp) {
        if (entry->d_name[0] == '.' && file_prefix[0] != '.')
            continue;
        
        if (strncmp(entry->d_name, file_prefix, prefix_len) == 0) {
            if (has_slash) {
                size_t total_len = strlen(prefix) - prefix_len + strlen(entry->d_name) + 1;
                char *full = malloc(total_len);
                snprintf(full, total_len, "%.*s%s", 
                    (int)(strlen(prefix) - prefix_len), prefix, entry->d_name);
                completions[count++] = full;
            } else {
                completions[count++] = strdup(entry->d_name);
            }
        }
    }
    
    closedir(d);
    free(expanded);
    return count;
}

int compare_strings(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

int handle_tab_completion(char *line_buffer, int *len) {
    if (*len == 0) return 0;
    
    char temp_line[MAX_LINE];
    strncpy(temp_line, line_buffer, *len);
    temp_line[*len] = '\0';
    
    char *last_space = strrchr(temp_line, ' ');
    char *word_start = last_space ? (last_space + 1) : temp_line;
    int word_pos = word_start - temp_line;
    
    char *completions[MAX_COMPLETIONS];
    int comp_count = 0;
    
    if (word_pos == 0) {
        comp_count = find_executables_in_path(word_start, completions, MAX_COMPLETIONS);
    } else {
        comp_count = find_file_completions(word_start, completions, MAX_COMPLETIONS);
    }
    
    if (comp_count == 0) {
        return 0;
    }
    
    qsort(completions, comp_count, sizeof(char *), compare_strings);
    
    if (comp_count == 1) {
        int old_word_len = strlen(word_start);
        int new_word_len = strlen(completions[0]);
        
        for (int i = 0; i < old_word_len; i++) {
            write(STDOUT_FILENO, "\b \b", 3);
        }
        
        strcpy(line_buffer + word_pos, completions[0]);
        *len = word_pos + new_word_len;
        line_buffer[*len] = '\0';
        
        write(STDOUT_FILENO, completions[0], new_word_len);
        
        free(completions[0]);
        return 1;
    }
    
    size_t common_len = strlen(completions[0]);
    for (int i = 1; i < comp_count; i++) {
        size_t j = 0;
        while (j < common_len && completions[0][j] == completions[i][j]) {
            j++;
        }
        common_len = j;
    }
    
    if (common_len > strlen(word_start)) {
        int old_word_len = strlen(word_start);
        
        for (int i = 0; i < old_word_len; i++) {
            write(STDOUT_FILENO, "\b \b", 3);
        }
        
        strncpy(line_buffer + word_pos, completions[0], common_len);
        *len = word_pos + common_len;
        line_buffer[*len] = '\0';
        
        write(STDOUT_FILENO, line_buffer + word_pos, common_len);
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
    
    return (comp_count > 1 && common_len <= strlen(word_start)) ? 2 : 1;
}

void add_to_history(const char *line) {
    char *temp = strdup(line);
    char *trimmed = temp;
    while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
    if (*trimmed == '\0') {
        free(temp);
        return;
    }
    
    if (history_count > 0 && strcmp(history[history_count-1], line) == 0) {
        free(temp);
        return;
    }
    free(temp);
    
    if (history_count < MAX_HISTORY) {
        history[history_count++] = strdup(line);
    } else {
        free(history[0]);
        for (int i = 1; i < MAX_HISTORY; i++) {
            history[i-1] = history[i];
        }
        history[MAX_HISTORY-1] = strdup(line);
    }
    history_current = history_count;
}

void load_history(const char *hist_file) {
    FILE *f = fopen(hist_file, "r");
    if (!f) return;
    
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) > 0 && line[0] != '#') {
            if (history_count < MAX_HISTORY) {
                history[history_count++] = strdup(line);
            }
        }
    }
    fclose(f);
    history_current = history_count;
}

void save_history(const char *hist_file) {
    FILE *f = fopen(hist_file, "w");
    if (!f) {
        perror(" Quantis: Error while attempting to save history.\n");
        return;
    }
    for (int i = 0; i < history_count; i++) {
        fprintf(f, "%s\n", history[i]);
    }
    fclose(f);
}

char *read_command_line(int prompt_len) {
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
            
            if (seq[0] == '[' && (seq[1] == 'A' || seq[1] == 'B')) {
                int new_history_index = history_current;
                
                if (seq[1] == 'A') {
                    if (new_history_index > 0) new_history_index--;
                } else if (seq[1] == 'B') {
                    if (new_history_index < history_count) new_history_index++;
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
                        len = snprintf(line_buffer, MAX_LINE, "%s", history_line);
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

char *extract_alias_value(const char *input) {
    char *open_brace = strchr(input, '{');
    char *close_brace = strrchr(input, '}');
    
    if (!open_brace || !close_brace || open_brace >= close_brace)
        return NULL;
    
    size_t len = close_brace - open_brace - 1;
    char *value = malloc(len + 1);
    if (value) {
        strncpy(value, open_brace + 1, len);
        value[len] = '\0';
    }
    return value;
}

void add_alias(const char *name, const char *value) {
    if (alias_count >= MAX_ALIASES) {
        fprintf(stderr, " Quantis: Too many aliases.\n");
        return;
    }
    
    char *value_copy = strdup(value);
    
    for (int i = 0; i < alias_count; i++) {
        if (strcmp(aliases[i].name, name) == 0) {
            free(aliases[i].value);
            aliases[i].value = value_copy;
            return;
        }
    }
    
    aliases[alias_count].name = strdup(name);
    aliases[alias_count].value = value_copy;
    alias_count++;
}

void remove_alias(const char *name) {
    for (int i = 0; i < alias_count; i++) {
        if (strcmp(aliases[i].name, name) == 0) {
            free(aliases[i].name);
            free(aliases[i].value);
            for (int j = i; j < alias_count - 1; j++) {
                aliases[j] = aliases[j+1];
            }
            alias_count--;
            return;
        }
    }
}

void save_aliases(const char *rc_file) {
    FILE *f = fopen(rc_file, "w");
    if (!f) return;
    
    fprintf(f, "# .qnrc\n# Quantis RC file\n\n# This file is used for storing created aliases.\n# It is not recommended to manually change the contents of this file.\n\n# Use the builtin alias and unalias commands to modify your aliases.\n\n");
    for (int i = 0; i < alias_count; i++) {
        fprintf(f, "alias %s:{%s}\n", aliases[i].name, aliases[i].value);
    }
    fclose(f);
}

char *expand_aliases(const char *input_line) {
    char *temp_line = strdup(input_line);
    if (!temp_line) return strdup(input_line);
    
    char *first_word = strtok(temp_line, " \t");
    
    if (!first_word) {
        free(temp_line);
        return strdup(input_line);
    }
    
    for (int i = 0; i < alias_count; i++) {
        if (strcmp(first_word, aliases[i].name) == 0) {
            const char *alias_value = aliases[i].value;
            size_t len = strlen(alias_value);
            
            size_t first_word_len = strlen(first_word);
            const char *rest_of_line = input_line + first_word_len;
            
            while (*rest_of_line == ' ' || *rest_of_line == '\t') {
                rest_of_line++;
            }
            
            size_t rest_len = strlen(rest_of_line);
            size_t expanded_len = len + (rest_len > 0 ? 1 : 0) + rest_len + 1;
            
            char *expanded = malloc(expanded_len);
            if (!expanded) {
                perror("malloc for expanded alias");
                free(temp_line);
                return strdup(input_line);
            }
            
            strcpy(expanded, alias_value);
            
            if (rest_len > 0) {
                strcat(expanded, " ");
                strcat(expanded, rest_of_line);
            }
            
            free(temp_line);
            return expanded;
        }
    }
    
    free(temp_line);
    return strdup(input_line);
}

void load_aliases(const char *rc_file) {
    FILE *f = fopen(rc_file, "r");
    if (!f) return;
    
    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0;
        char *trimmed = line;
        while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
        
        if (strlen(trimmed) == 0 || trimmed[0] == '#') continue;
        
        if (strncmp(trimmed, "alias ", 6) == 0) {
            char *alias_def = trimmed + 6;
            char *colon = strchr(alias_def, ':');
            if (!colon) continue;
            
            *colon = '\0';
            char *name = alias_def;
            
            char *name_end = name + strlen(name) - 1;
            while (name_end >= name && (*name_end == ' ' || *name_end == '\t'))
                name_end--;
            *(name_end + 1) = '\0';
            
            char *value = extract_alias_value(colon + 1);
            
            if (strlen(name) > 0 && value) {
                add_alias(name, value);
            }
            if (value) free(value);
        }
    }
    fclose(f);
}

int parse_line(char *line, char **argv, int *bg) {
    int argc = 0;
    char *t;
    *bg = 0;
    
    t = strtok(line, " \t");
    while (t && argc < MAX_ARGS - 1) {
        if (strcmp(t, "&") == 0) {
            *bg = 1;
            break;
        } else {
            argv[argc++] = t;
        }
        t = strtok(NULL, " \t");
    }
    
    argv[argc] = NULL;
    return argc;
}

int handle_builtin(char **argv, char *rc_file, char *hist_file) {
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
        return 1;
    }
    if (!strcmp(argv[0], "alias")) {
        if (!argv[1]) {
            for (int i = 0; i < alias_count; i++) {
                printf("Alias %i : %s  %s\n", (i+1), aliases[i].name, aliases[i].value);
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
                if (argv[i+1]) strcat(alias_def, " ");
                i++;
            }
            
            char *colon = strchr(alias_def, ':');
            if (!colon) {
                fprintf(stderr, " Quantis: alias: Usage: alias name:{alias name}\n");
                free(alias_def);
                return 1;
            }
            
            *colon = '\0';
            char *name = alias_def;
            
            char *name_end = name + strlen(name) - 1;
            while (name_end >= name && (*name_end == ' ' || *name_end == '\t'))
                name_end--;
            *(name_end + 1) = '\0';
            
            char *value = extract_alias_value(colon + 1);
            
            if (strlen(name) > 0 && value) {
                add_alias(name, value);
                save_aliases(rc_file);
            } else {
                fprintf(stderr, " Quantis: alias: Invalid value extraction.\n");
            }
            
            if (value) free(value);
            free(alias_def);
        }
        return 1;
    }
    if (!strcmp(argv[0], "unalias")) {
        if (!argv[1]) {
            fprintf(stderr, " Quantis: unalias: Usage: unalias name\n");
        } else {
            remove_alias(argv[1]);
            save_aliases(rc_file);
        }
        return 1;
    }
    return 0;
}

void execute_command(char **argv, int bg) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }
    if (pid == 0) {
        signal(SIGINT, SIG_DFL);
        execvp(argv[0], argv);
        fprintf(stderr, " Quantis: %s: %s\n", argv[0], strerror(errno));
        _exit(127);
    }
    if (bg)
        printf("[%d] %d\n", getpid(), pid);
    else {
        child_pid = pid;
        waitpid(pid, NULL, 0);
        child_pid = 0;
    }
}

void ensure_file(const char *path, const char *def) {
    int fd = open(path, O_CREAT | O_EXCL | O_WRONLY, 0644);
    if (fd >= 0) {
        write(fd, def, strlen(def));
        close(fd);
    }
}

char *get_program_directory() {
    char path[PATH_BUF];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    
    if (len != -1) {
        path[len] = '\0';
        char *last_slash = strrchr(path, '/');
        if (last_slash) *last_slash = '\0';
    } else {
        if (!getcwd(path, sizeof(path))) {
            return strdup(".");
        }
    }
    return strdup(path);
}

int main(int argc, char *argv[]) {
    signal(SIGINT, sigint_handler);
    
    if (argc > 1) {
        if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
            printf("\n" FG_CYAN "Quantis " COL_RESET "version " _VER "\n\n");
            
            struct passwd *pw = getpwuid(getuid());
            printf("Usage  Quantis [OPTIONS]\n\n");
            printf("Options \n");
            printf("  --help, -h      Show this help message\n");
            printf("  --version, -v   Show version information\n\n");
            printf("User  %s\n", pw ? pw->pw_name : "unknown");
            printf("UID  %d\n\n", getuid());
            
            return 0;
        } else if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
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
            fprintf(stderr, "Quantis: unknown option: %s\n", argv[1]);
            fprintf(stderr, "Try 'Quantis --help' for more information.\n");
            return 1;
        }
    }
    
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
    if (!getenv("COLORTERM")) setenv("COLORTERM", "truecolor", 1);
    
    printf("\033c");
    load_aliases(rc);
    load_history(hist);
    
    while (run) {
        prompt = build_prompt();
        printf("%s", prompt);
        fflush(stdout);
        
        int prompt_len = 8 + strlen(getpwuid(getuid())->pw_name) + strlen(getcwd(NULL, 0));
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
    printf("\n  Exiting Quantis...\n\n");
    return 0;
}