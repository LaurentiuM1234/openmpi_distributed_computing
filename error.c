#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "error.h"

void printerr(const char *msg, int code)
{
        if (code == -ELIB) {
                fprintf(stderr, "[ERROR] %s: %s\n", msg, strerror(errno));
	} else if (code == -EREAD) {
                fprintf(stderr, "[ERROR] %s: Failed to read values.\n", msg);
        } else {
                fprintf(stderr, "[ERROR] Unknown error code.\n");
        }
}
