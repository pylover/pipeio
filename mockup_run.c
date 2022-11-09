#include "mockup.h"

#include <clog.h>
#include <stdlib.h>
#include <stdio.h>


int main() {
    char *outbuff = "Hello\n";
    size_t size = 6;
    char inbuff[size];
    
    struct unixsrv *s = unixsrv_fork("/tmp/pipeio-foo.s", outbuff, inbuff, 
            size);
    if (s == NULL) {
        FATAL("fork_unixserver");
    }

    INFO("Server started");
    if (unixsrv_wait(s, inbuff, size)) {
        FATAL("unixsrv_wait");
    }
    INFO("Reply: %.*s", 6, inbuff);
    return EXIT_SUCCESS;
}
