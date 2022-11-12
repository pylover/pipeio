#include "pipeio.h"

#include <clog.h>
#include <mrb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


struct pipeio_side {
    struct pipeio_task task;

    pipeio_worker writer;
    pipeio_worker reader;

    struct pipeio *backref;
};


struct pipeio {
    /* Status */
    bool closing;

    /* sides */
    struct pipeio_side inside;
    struct pipeio_side outside;

    pipeio_callback errhandler;

    /* Buffers */
    struct mrb input;
    struct mrb output;

    /* Options */
    size_t mtu;
    void *backref;
};


static int
_readwrite(int fd, int op, struct pipeio_side *s) {
    struct pipeio *p = s->backref;

    if (op & PIO_RDHUP) {
        INFO("Remote hanged up");
        p->errhandler(fd, op, p);
    }

    if (op & PIO_ERR) {
        INFO("Connection error");
        p->errhandler(fd, op, p);
    }

    if (op & PIO_READ) {

    }

    if (op & PIO_WRITE) {

    }
}


struct pipeio *
pipeio_create(int infd, int outfd, size_t mtu, size_t buffsize, void *backref) {
    struct pipeio *p = malloc(sizeof(struct pipeio));
    if (p == NULL) {
        return NULL;
    }

    if (mrb_init(&(p->input), buffsize)) {
        free(p);
        return NULL;
    }

    if (mrb_init(&(p->output), buffsize)) {
        mrb_deinit(&(p->input));
        free(p);
        return NULL;
    }

    p->closing = false;
    p->backref = backref;

    /* tasks */
    p->inside.task.fd = infd;
    p->inside.task.op = PIO_READ;
    p->inside.task.callback = (pipeio_callback)_readwrite;
    p->inside.task.arg = p;

    p->outside.task.fd = outfd;
    p->outside.task.op = PIO_READ;
    p->outside.task.callback = (pipeio_callback)_readwrite;
    p->outside.task.arg = p;

    /* Callbacks */
    p->inside.writer = pipeio_writer;
    p->inside.reader = pipeio_reader;
    p->outside.writer = pipeio_writer;
    p->outside.reader = pipeio_reader;
    p->errhandler = NULL;
    
    return p;
}


void
pipeio_destroy(struct pipeio *p) {
    if (p == NULL) {
        return;
    }
    mrb_deinit(&(p->input));
    mrb_deinit(&(p->output));
    free(p);
}


void
pipeio_start(struct pipeio *p) {
    _readwrite(p->inside.task.fd, p->inside.task.op, &(p->inside));
    _readwrite(p->outside.task.fd, p->outside.task.op, &(p->outside));
}


void
pipeio_inside_writer_set(struct pipeio *p, pipeio_worker writer) {
    p->inside.writer = writer;
}


void
pipeio_inside_reader_set(struct pipeio *p, pipeio_worker reader) {
    p->inside.reader = reader;
}


void
pipeio_outside_writer_set(struct pipeio *p, pipeio_worker writer) {
    p->outside.writer = writer;
}


void
pipeio_outside_reader_set(struct pipeio *p, pipeio_worker reader) {
    p->outside.reader = reader;
}


enum pipeio_status
pipeio_reader(struct pipeio *p, int fd, struct mrb *buff) {
    return PIO_ERROR;
}


enum pipeio_status
pipeio_writer(struct pipeio *p, int fd, struct mrb *buff) {
    return PIO_ERROR;
}
