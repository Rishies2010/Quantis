#include "quantis.h"

char *build_prompt(void) {
    char buf[PROMPT_BUF], cwd[PATH_MAX];
    struct passwd *pw = getpwuid(getuid());
    const char *user = pw ? pw->pw_name : "user";

    if (!getcwd(cwd, sizeof(cwd)))
        strcpy(cwd, "?");

    snprintf(buf, sizeof(buf),
             "\n" 
                             "  " BG_BLACK FG_CYAN "" COL_RESET
                                 BG_CYAN FG_BLACK " %s " COL_RESET
                                     BG_BLACK FG_CYAN "" COL_RESET
                             " > ",
             cwd);
    return strdup(buf);
}

