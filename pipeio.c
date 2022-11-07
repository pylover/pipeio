#include "pipeio.h"

#include <stdio.h>
#include <stdbool.h>


struct pipeio {
    bool closing;

    /* Workers */
    struct wepn_iotask *left;
    struct wepn_iotask *right;

    /* Callbacks */
    pipeio_worker left_writer;
    pipeio_worker left_reader;
    pipeio_worker right_writer;
    pipeio_worker right_reader;

    /* Buffers */
    struct mrb *input;
    struct mrb *output;

    /* Options */
    size_t mtu;
};


struct pipeio *
pipeio_create(int lfd, int rfd, size_t mtu, size_t buffsize) {
    return NULL;
}
