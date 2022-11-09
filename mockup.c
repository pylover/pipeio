#include "mockup.h"

#include <clog.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <sys/un.h>


#define EV_SHOULDWAIT() ((errno == EAGAIN) || (errno == EWOULDBLOCK))


int
unix_connect(const char *filename) {
    int fd;
    struct sockaddr_un addr;

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        ERROR("socket");
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, filename);
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        ERROR("connect");
        return -1;
    }
    
    return fd;
}


int
unixsrv_start(const char *filename, char *outbuff, char *inbuff, 
        size_t size) {
    int listenfd;
    int connfd;
    struct sockaddr_un addr, peer_addr;
    socklen_t peer_addr_size;

    /* unlink previous link to file */
    unlink(filename);

    listenfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listenfd == -1) {
        FATAL("socket");
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, filename, sizeof(addr.sun_path) - 1);

    if (bind(listenfd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        FATAL("bind");
    }

    if (listen(listenfd, 1) == -1) {
        FATAL("listen");
    }

    /* accept */
    peer_addr_size = sizeof(peer_addr);
    connfd = accept4(
            listenfd, 
            (struct sockaddr *) &peer_addr,
            &peer_addr_size,
            SOCK_NONBLOCK
        );
    if (connfd == -1) {
        FATAL("accept");
    }
    
    /* Event Loop */
    int epollfd = epoll_create1(0);
    struct epoll_event ev;
    struct epoll_event events[1];
    int evcount;
    int i;
    int fd;
    size_t wlen = 0;
    size_t rlen = 0;
    ssize_t tmp;

    ev.events = EPOLLET | EPOLLRDHUP | EPOLLERR | EPOLLIN | EPOLLOUT;
    ev.data.fd = connfd;

    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev) != 0) {
        FATAL("epoll_ctl");
    }

    while (true) {
        evcount = epoll_wait(epollfd, events, 1, -1);
        if (evcount < 0) {
            FATAL("epoll_wait");
        }

        if (evcount == 0) {
            break;
        }

        for (i = 0; i < evcount; i++) {
            fd = events[i].data.fd;

            if (events[i].events & EPOLLIN) {
                /* read */
                errno = 0;
                tmp = recv(fd, inbuff + rlen, size - rlen, 0);
                if (tmp == 0) {
                    FATAL("recv EOF");
                }
                if ((tmp < 0) && (!EV_SHOULDWAIT())) {
                    FATAL("recv");
                }
                rlen += tmp;
            }
            else if (events[i].events & EPOLLOUT) {
                /* write */
                errno = 0;
                tmp = send(fd, outbuff + wlen, size - wlen, 0);
                if (tmp == 0) {
                    FATAL("send EOF");
                }

                if ((tmp < 0) && (!EV_SHOULDWAIT())) {
                    FATAL("send");
                }
                wlen += tmp;
            }
            else if (events[i].events & EPOLLRDHUP) {
                /* hangup */
                close(fd);
                FATAL("Remote hanged up");
            }
            else if (events[i].events & EPOLLERR) {
                /* error */
                close(fd);
                FATAL("Connection error");
            }
        }

        ev.data.fd = connfd;
        ev.events = EPOLLET | EPOLLRDHUP | EPOLLERR;
        if ((wlen >= size) && (rlen >= size)) {
            break;
        }
        if (wlen < size) {
            ev.events |= EPOLLOUT;
        }
        if (rlen < size) {
            ev.events |= EPOLLIN;
        }
        if (epoll_ctl(epollfd, EPOLL_CTL_MOD, connfd, &ev) != 0) {
            FATAL("epoll_ctl");
        }
    }

    close(connfd);
    close(listenfd);
    close(epollfd);
    unlink(filename);
    return 0;
}


struct unixsrv *
unixsrv_fork(const char *filename, char *outbuff, char *inbuff, size_t size) {
    struct unixsrv *s = malloc(sizeof(struct unixsrv));
    if (pipe(s->pipe) == -1) {
        FATAL("pipe");
    }

    pid_t childpid = fork();
    if (childpid == 0) {
        /* Child process */
        close(s->pipe[0]);
        if (unixsrv_start(filename, outbuff, inbuff, size)) {
            ERROR("Cannot start unixserver");
            exit(1);
        }

        write(s->pipe[1], inbuff, size);
        close(s->pipe[1]);
        exit(0);
        return NULL;
    }

    /* Parent process */
    s->pid = childpid;
    return s;
}


int
unixsrv_wait(struct unixsrv *s, char *inbuff, size_t size) {
    int status;
    wait(&status);
    close(s->pipe[1]);
    read(s->pipe[0], inbuff, size);
    close(s->pipe[0]);
    free(s);
    return status;
}
