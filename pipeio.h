#ifndef PIPEIO_H
#define PIPEIO_H


#include <stdio.h>


struct pipeio;


enum pipeio_status {
    IOR_OK = 0,
    IOR_AGAIN = -1,
    IOR_ERROR = -2,
    IOR_BUFFERFULL = -3,
    IOR_MOREDATA = -4,
};


typedef enum pipeio_status (*pipeio_worker) (struct pipeio *);


struct pipeio *
pipeio_create(int lfd, int rfd, size_t mtu, size_t buffsize);


void
pipeio_destroy(struct pipeio *tw);


#endif
