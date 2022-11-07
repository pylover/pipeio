#include "pipeio.h"

#include <mrb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


struct pipeio {
    /* Status */
    bool closing;

    /* Workers */
    struct pipeio_iotask left;
    struct pipeio_iotask right;

    /* Callbacks */
    pipeio_worker leftwriter;
    pipeio_worker leftreader;
    pipeio_worker rightwriter;
    pipeio_worker rightreader;
    pipeio_worker close;

    /* Buffers */
    struct mrb input;
    struct mrb output;

    /* Options */
    size_t mtu;
    void *backref;
};


struct pipeio *
pipeio_create(int lfd, int rfd, size_t mtu, size_t buffsize, void *backref) {
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

    /* Callbacks */
    p->leftwriter = pipeio_leftwriter;
    p->leftreader = pipeio_leftreader;
    p->rightwriter = pipeio_rightwriter;
    p->rightreader = pipeio_rightreader;
    p->close = NULL;
    
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
