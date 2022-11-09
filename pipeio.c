#include "pipeio.h"

#include <mrb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


struct pipeio {
    /* Status */
    bool closing;

    /* Workers */
    struct pipeio_task inside;
    struct pipeio_task outside;

    /* Callbacks */
    pipeio_worker leftwriter;
    pipeio_worker leftreader;
    pipeio_worker rightwriter;
    pipeio_worker rightreader;
    pipeio_errhandler errhandler;

    /* Buffers */
    struct mrb input;
    struct mrb output;

    /* Options */
    size_t mtu;
    void *backref;
};


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
    p->inside.fd = infd;
    p->outside.fd = outfd;

    /* Callbacks */
    p->leftwriter = pipeio_writer;
    p->leftreader = pipeio_reader;
    p->rightwriter = pipeio_writer;
    p->rightreader = pipeio_reader;
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
pipeio_leftwriter_set(struct pipeio *p, pipeio_worker writer) {
    p->leftwriter = writer;
}


void
pipeio_leftreader_set(struct pipeio *p, pipeio_worker reader) {
    p->leftreader = reader;
}


void
pipeio_rightwriter_set(struct pipeio *p, pipeio_worker writer) {
    p->rightwriter = writer;
}


void
pipeio_rightreader_set(struct pipeio *p, pipeio_worker reader) {
    p->rightreader = reader;
}


enum pipeio_status
pipeio_reader(struct pipeio *p, int fd, struct mrb *buff) {
    return IOR_OK;
}


enum pipeio_status
pipeio_writer(struct pipeio *p, int fd, struct mrb *buff) {
    return IOR_OK;
}
