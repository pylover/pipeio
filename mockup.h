#ifndef MOCKUP_H
#define MOCKUP_H


#include <sys/types.h>


struct unixsrv {
    pid_t pid;
    int pipe[2];
};


struct unixsrv *
unixsrv_fork(const char *filename, char *outbuff, char *inbuff, size_t size);


int
unixsrv_wait(struct unixsrv *s, char *inbuff, size_t size);


#endif
