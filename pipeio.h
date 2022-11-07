#ifndef PIPEIO_H
#define PIPEIO_H


#include <stdio.h>
#include <errno.h>
#include <sys/epoll.h>


#define EV_MAXEVENTS  16
#define EV_SHOULDWAIT() ((errno == EAGAIN) || (errno == EWOULDBLOCK))


enum pipeio_ioop {
    WIO_READ = EPOLLIN,
    WIO_WRITE = EPOLLOUT,
    WIO_RDHUP = EPOLLRDHUP,
    WIO_ERR = EPOLLERR,
};


typedef void (*pipeio_task)(void *arg, int events);


/* A simple bag which used by waitA to hold io's essential data 
   until the underlying file descriptor becomes ready for read or write. */
struct pipeio_iotask {
    int fd;
    enum pipeio_ioop op;
    void *arg;
    pipeio_task callback;
};


void 
pipeio_init(int flags);


void 
pipeio_deinit();


int 
pipeio_arm(struct pipeio_iotask *task);


int 
pipeio_dearm(int fd);


void
pipeio_loop(volatile int *status, int holdfd);



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
pipeio_create(int lfd, int rfd, size_t mtu, size_t buffsize, void *backref);


void
pipeio_destroy(struct pipeio *tw);


void
pipeio_leftwriter_set(struct pipeio *p, pipeio_worker writer);

#endif
