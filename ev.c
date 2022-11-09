#include "pipeio.h"

#include <clog.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>


static int _epfd = -1;
static int _epflags = EPOLLONESHOT | EPOLLET | EPOLLRDHUP | EPOLLERR;
static struct epoll_event *_events = NULL;
static volatile int _tasks = 0;


#define EV_MAXEVENTS  16
#define _MAXFILES  (EV_MAXEVENTS + 4)


static int _files[_MAXFILES];


void
pipeio_init(int flags) {
    int i;

    if (_epfd != -1) {
        return;
    }
    _epflags |= flags;
    _epfd = epoll_create1(0);
    if (_epfd < 0) {
        FATAL("epoll_create1");
    }

    _events = calloc(EV_MAXEVENTS, sizeof(struct epoll_event));
    if (_events == NULL) {
        FATAL("Out of memory");
    }
    
    for (i = 0; i < _MAXFILES; i++) {
        _files[i] = -1;
    }
}


void
pipeio_deinit() {
    close(_epfd);
    free(_events);
}


int
pipeio_arm(struct pipeio_task *task) {
    int fd = task->fd;
    struct epoll_event ev;

    ev.events = _epflags | task->op;
    ev.data.ptr = task;

    if (epoll_ctl(_epfd, EPOLL_CTL_MOD, fd, &ev) != 0) {
        if (errno == ENOENT) {
            errno = 0;
            /* File descriptor is not exists yet */
            if (epoll_ctl(_epfd, EPOLL_CTL_ADD, fd, &ev) != 0) {
                return -1;
            }
        }
        else {
            return -1;
        }
    }
    
    if (_files[fd] == -1) {
        _tasks++;
        _files[fd] = fd;
    }
    return 0;
}


int
pipeio_dearm(int fd) {
    if (epoll_ctl(_epfd, EPOLL_CTL_DEL, fd, NULL) != 0) {
        return -1;
    }
    if (_files[fd] != -1) {
        _tasks--;
        _files[fd] = -1;
    }
    return 0;
}


void
pipeio_loop(volatile int *status, int holdfd) {
    int i;
    int fd;
    struct pipeio_task *task;
    int count;
    
    while (_tasks && ((status == NULL) || (*status > EXIT_FAILURE))) {
        count = epoll_wait(_epfd, _events, EV_MAXEVENTS, -1);
        if (count < 0) {
            ERROR("epoll_wait");
            errno = 0;
            *status = EXIT_FAILURE;
            break;
        }

        if (count == 0) {
            break;
        }

        for (i = 0; i < count; i++) {
            task = (struct pipeio_task *) _events[i].data.ptr;
            fd = task->fd;

            if ((fd != holdfd) && (_files[fd] != -1)) {
                _tasks--;
                _files[fd] = -1;
            }

            task->callback(task->arg, _events[i].events);
        }
    }
}
