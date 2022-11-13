#ifndef PIPEIO_H
#define PIPEIO_H


#include <mrb.h>
#include <stdio.h>
#include <errno.h>
#include <sys/epoll.h>


#define EV_MAXEVENTS  16
#define EV_SHOULDWAIT() ((errno == EAGAIN) || (errno == EWOULDBLOCK))


struct pipeio;


enum pipeio_status {
    PIO_UNKNOWN = 0,
    PIO_AGAIN = -1,
    PIO_ERROR = -2,
    PIO_BUFFERFULL = -3,
    PIO_MOREDATA = -4,
    PIO_EOF = -5,
};


enum pipeio_op {
    PIO_READ = EPOLLIN,
    PIO_WRITE = EPOLLOUT,
    PIO_RDHUP = EPOLLRDHUP,
    PIO_ERR = EPOLLERR,
};


typedef void (*pipeio_callback)(int fd, int events, void *arg);
typedef void (*pipeio_onerror)(struct pipeio *p);


typedef enum pipeio_status (*pipeio_worker) 
    (struct pipeio *, int fd, struct mrb *buff);


/* A simple bag which used by waitA to hold io's essential data 
   until the underlying file descriptor becomes ready for read or write. */
struct pipeio_task {
    int fd;
    enum pipeio_op op;
    void *arg;
    pipeio_callback callback;
};


void 
pipeio_init(int flags);


void 
pipeio_deinit();


int 
pipeio_arm(struct pipeio_task *task);


int 
pipeio_dearm(int fd);


void
pipeio_loop(volatile int *status, int holdfd);


struct pipeio *
pipeio_create(int lfd, int rfd, size_t mtu, size_t buffsize, void *backref);


void
pipeio_destroy(struct pipeio *p);


void
pipeio_inside_writer_set(struct pipeio *p, pipeio_worker writer);


void
pipeio_inside_reader_set(struct pipeio *p, pipeio_worker reader);


void
pipeio_outside_writer_set(struct pipeio *p, pipeio_worker writer);


void
pipeio_outside_reader_set(struct pipeio *p, pipeio_worker reader);


enum pipeio_status
pipeio_reader(struct pipeio *p, int fd, struct mrb *buff);


enum pipeio_status
pipeio_writer(struct pipeio *p, int fd, struct mrb *buff);


void
pipeio_start(struct pipeio *p);


struct pipeio_side*
pipeio_otherside(struct pipeio_side *s);


int
register_for_read(struct pipeio_side *s);


int
register_for_write(struct pipeio_side *s);


#endif
