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
        ERROR("open /dev/urandom");
        goto failure;
    }

    inside = unixsrv_fork("/tmp/pipeio-inside.s", inside_outbuff, 
            inside_inbuff, BUFFSIZE);
    if (inside == NULL) {
        ERROR("fork_unixserver inside");
        goto failure;
    }

    outside = unixsrv_fork("/tmp/pipeio-outside.s", outside_outbuff, 
                outside_inbuff, BUFFSIZE);
    if (outside == NULL) {
        ERROR("fork_unixserver outside");
        goto failure;
    }

    if (read(randfd, inside_outbuff, BUFFSIZE) <= 0) {
        ERROR("read random");
        goto failure;
    }
    if (read(randfd, outside_outbuff, BUFFSIZE) <= 0) {
        ERROR("read random");
        goto failure;
    }

    int infd = unix_connect("/tmp/pipeio-inside.s");
    if (infd < 0) {
        ERROR("unix_connect");
        goto failure;
    }

    int outfd = unix_connect("/tmp/pipeio-outside.s");
    if (outfd < 0) {
        ERROR("unix_connect");
        goto failure;
    }

    struct pipeio *pipe = pipeio_create(infd, outfd, 1024, BUFFSIZE, NULL);
    if (pipe == NULL) {
        ERROR("pipeio_create");
        goto failure;
    }

    unixsrv_wait(inside, inside_inbuff, BUFFSIZE);
    unixsrv_wait(outside, outside_inbuff, BUFFSIZE);

    /* compare */
    eqnstr(inside_outbuff, outside_inbuff, BUFFSIZE);
    eqnstr(outside_outbuff, inside_inbuff, BUFFSIZE);

    goto destroy;

failure:
    unixsrv_kill(inside);
    unixsrv_kill(outside);

destroy:
    /* free */
    pipeio_destory(pipe);
    free(inside_inbuff);
    free(inside_outbuff);
    free(outside_inbuff);
    free(outside_outbuff);
}


int main() {
    test_pipeio_create();
    return EXIT_SUCCESS;
}
