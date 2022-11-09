#include "pipeio.h"

#include <clog.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>


int
start_unixserver(const char *filename, char *outbuff, char *inbuff, 
        size_t size) {
    int listenfd;
    int connfd;
    struct sockaddr_un addr, peer_addr;
    socklen_t peer_addr_size;

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

    ev.events = EPOLLONESHOT | EPOLLET | EPOLLRDHUP | EPOLLERR | 
        EPOLLIN | EPOLLOUT;
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
            }
            else if (events[i].events & EPOLLOUT) {
                /* write */
            }
            else if (events[i].events & EPOLLRDHUP) {
                /* hangup */
            }
            else if (events[i].events & EPOLLERR) {
                /* error */
            }
        }
    }

    close(connfd);
    close(listenfd);
    close(epollfd);
    unlink(filename);
}


int
fork_unixserver(const char *filename, char *inbuff, char *outbuff, 
        size_t size) {
    pid_t childpid = fork();
    if (childpid == 0) {
        /* Child process */
        if (start_unixserver(filename, inbuff, outbuff, size)) {
            ERROR("Cannot start unixserver");
            exit(1);
        }
        exit(0);
        return 0;
    }

    /* Parent process */
    return childpid;
}


void
test_pipeio_create() {
    // struct pipeio *p = pipeio_create(
}


int main() {
    start_unixserver("/tmp/pipeio-foo.s", NULL, NULL, 0);
    return EXIT_SUCCESS;
}
