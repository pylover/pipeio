#include "pipeio.h"
#include "mockup.h"

#include <cutest.h>
#include <clog.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>


#define BUFFSIZE   4096


void
test_pipeio_create() {
    struct unixsrv *inside;
    struct unixsrv *outside;
    char *inside_inbuff = malloc(BUFFSIZE);
    char *inside_outbuff = malloc(BUFFSIZE);
    char *outside_inbuff = malloc(BUFFSIZE);
    char *outside_outbuff = malloc(BUFFSIZE);
    int randfd = open("/dev/urandom", O_RDONLY);
    if (randfd < 0) {
        FATAL("open /dev/urandom");
    }

    inside = unixsrv_fork("/tmp/pipeio-inside.s", inside_outbuff, 
            inside_inbuff, BUFFSIZE);
    if (inside == NULL) {
        FATAL("fork_unixserver inside");
    }

    outside = unixsrv_fork("/tmp/pipeio-outside.s", outside_outbuff, 
                outside_inbuff, BUFFSIZE);
    if (outside == NULL) {
        FATAL("fork_unixserver outside");
    }

    unixsrv_wait(inside, inside_inbuff, BUFFSIZE);
    unixsrv_wait(outside, outside_inbuff, BUFFSIZE);
    free(inside_inbuff);
    free(inside_outbuff);
    free(outside_inbuff);
    free(outside_outbuff);
}


int main() {
    test_pipeio_create();
    return EXIT_SUCCESS;
}
