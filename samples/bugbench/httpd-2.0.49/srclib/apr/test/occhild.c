#include "apr.h"
#include <stdio.h>
#if APR_HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h>

int main(void)
{
    char buf[256];

    while (1) {
        read(STDERR_FILENO, buf, 256);
    }
    return 0; /* just to keep the compiler happy */
}
