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

    struct mrb *readbuff;
    struct mrb *writebuff;

    struct pipeio *pipe;
};


struct pipeio {
    /* Status */
    bool closing;

    /* sides */
    struct pipeio_side inside;
    struct pipeio_side outside;

    pipeio_onerror errhandler;

    /* Buffers */
    struct mrb *input;
    struct mrb *output;

    /* Options */
    void *backref;
};


static int
_read(struct pipeio_side *s) {
    int fd = s->task.fd;
    struct pipeio *p = s->pipe;
    enum pipeio_status status = s->reader(p, fd, s->readbuff);
    DEBUG("read: %d", status);
    switch (status) {
        case PIO_BUFFERFULL:
            if (register_for_write(pipeio_otherside(s)) < 0) {
                ERROR("register_for_write(otherside)");
                return -1;
            }
            break;
        case PIO_AGAIN:
            if (register_for_read(s) < 0) {
                ERROR("register_for_read");
                return -1;
            }
            break;

        case PIO_EOF:
            INFO("Remote hanged up");
            return -1;

        case PIO_ERROR:
            ERROR("Connection error");
            return -1;

        default:
            ERROR("Unknown situation, status: %d", status);
            return -1;
    }
    return 0;
}


static int
_write(struct pipeio_side *s) {
    int fd = s->task.fd;
    struct pipeio *p = s->pipe;
    enum pipeio_status status = s->writer(p, fd, s->writebuff);
    DEBUG("write: %d", status);
    switch (status) {
        case PIO_MOREDATA:
            struct pipeio_side *otherside = pipeio_otherside(s);
            if (_read(otherside)) {
                return -1;
            }
            break;
        case PIO_AGAIN:
            if (register_for_write(s) < 0) {
                ERROR("register_for_write");
                return -1;
            }
            break;

        case PIO_EOF:
            INFO("Remote hanged up");
            return -1;

        case PIO_ERROR:
            ERROR("Connection error");
            return -1;

        default:
            ERROR("Unknown situation, status: %d", status);
            return -1;
    }
    return 0;
}


static void
_readwrite(int fd, int op, struct pipeio_side *s) {
    struct pipeio *p = s->pipe;
    enum pipeio_status status;
    
    if (op & PIO_RDHUP) {
        INFO("Remote hanged up");
        p->errhandler(p);
        return;

    }

    if (op & PIO_ERR) {
        ERROR("Connection");
        p->errhandler(p);
        return;
    }

    if ((op & PIO_READ) && _read(s)) {
        ERROR("_read");
        p->errhandler(p);
        return;
    }

    if ((op & PIO_WRITE) && _write(s)) {
        ERROR("_write");
        p->errhandler(p);
        return;
    }
}


enum pipeio_status
pipeio_reader(struct pipeio *p, int fd, struct mrb *buff) {
    size_t toread = mrb_space_available(buff); 
    ssize_t result;

    while (toread) {
        result = mrb_readin(buff, fd, toread);
        DEBUG("readin: %d, len: %lu", fd, result);
        if (result < 0) {
            if (EV_SHOULDWAIT()) {
                return PIO_AGAIN;
            }
            return PIO_ERROR;
        }

        if (result == 0) {
            return PIO_EOF;
        }

        toread -= result;
    }
    return PIO_BUFFERFULL;
}


enum pipeio_status
pipeio_writer(struct pipeio *p, int fd, struct mrb *buff) {
    size_t towrite = mrb_space_used(buff); 
    ssize_t result;

    while (towrite) {
        result = mrb_writeout(buff, fd, towrite);
        DEBUG("writeout: %d, len: %lu", fd, result);
        if (result < 0) {
            if (EV_SHOULDWAIT()) {
                return PIO_AGAIN;
            }
            return PIO_ERROR;
        }

        if (result == 0) {
            return PIO_EOF;
        }

        towrite -= result;
    }
    return PIO_MOREDATA;
}


struct pipeio *
pipeio_create(int infd, int outfd, size_t buffsize, void *backref) {
    struct pipeio *p = malloc(sizeof(struct pipeio));
    if (p == NULL) {
        return NULL;
    }

    /* Buffers */
    p->input = mrb_create(buffsize);
    if (p->input == NULL) {
        free(p);
        return NULL;
    }

    p->output = mrb_create(buffsize);
    if (p->output == NULL) {
        mrb_destroy(p->input);
        free(p);
        return NULL;
    }

    p->inside.readbuff = p->output;
    p->inside.writebuff = p->input;

    p->outside.readbuff = p->input;
    p->outside.writebuff = p->output;

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

    /* Error handler */
    p->errhandler = NULL;

    /* Miscellaneous */
    p->closing = false;
    p->backref = backref;
    p->inside.pipe = p;
    p->outside.pipe = p;

    return p;
}


void
pipeio_destroy(struct pipeio *p) {
    if (p == NULL) {
        return;
    }
    mrb_destroy(p->input);
    mrb_destroy(p->output);
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


void
pipeio_errhandler_set(struct pipeio *p, pipeio_onerror handler) {
    p->errhandler = handler;
}


int
register_for_read(struct pipeio_side *s) {
    if (!mrb_isfull(s->readbuff)) {
        return 1;
    }
  
    bool write = s->task.op & PIO_WRITE;
    s->task.op = PIO_READ;
    if (write) {
        s->task.op |= PIO_WRITE;
    }

    if (pipeio_arm(&(s->task))) {
        return -1;
    }
    return 0;
}


int
register_for_write(struct pipeio_side *s) {
    if (mrb_isempty(s->writebuff)) {
        return 1;
    }
  
    bool read = s->task.op & PIO_READ;
    s->task.op = PIO_WRITE;
    if (read) {
        s->task.op |= PIO_READ;
    }

    if (pipeio_arm(&(s->task))) {
        return -1;
    }
    return 0;
}


struct pipeio_side*
pipeio_otherside(struct pipeio_side *s) {
    struct pipeio *p = s->pipe;
    struct pipeio_side *inside = &(p->inside);
    struct pipeio_side *outside = &(p->outside);

    if (inside == s) {
        return outside;
    }
    return inside;
}
