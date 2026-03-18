#include "quantis.h"

void save_aliases(const char *rc_file) {
    FILE *f = fopen(rc_file, "w");
    if (!f) {
        /* Same reasoning as history: if the config file cannot be
         * created (e.g. read-only FS), just skip saving aliases. */
        return;
    }

    fprintf(f,
            "# .qnrc\n"
            "# Quantis RC file\n\n"
            "# This file is used for storing created aliases.\n"
            "# It is not recommended to manually change the contents of this file.\n\n"
            "# Use the builtin alias and unalias commands to modify your aliases.\n\n");
    for (int i = 0; i < alias_count; i++) {
        fprintf(f, "alias %s:{%s}\n",
                aliases[i].name, aliases[i].value);
    }
    fclose(f);
}
