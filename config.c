#include "quantis.h"

void ensure_file(const char *path, const char *def) {
    int fd = open(path, O_CREAT | O_EXCL | O_WRONLY, 0644);
    if (fd >= 0) {
        write(fd, def, strlen(def));
        close(fd);
    }
}

