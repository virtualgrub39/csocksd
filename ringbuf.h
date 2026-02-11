#ifndef RINGBUF_H
#define RINGBUF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>

typedef struct
{
    size_t size;
    size_t head;
    size_t tail;
    size_t count;
    uint8_t *data;
} ringbuf;

ringbuf *ringbuf_new (size_t capacity);
void ringbuf_free (ringbuf *r);

size_t ringbuf_len (const ringbuf *r);
size_t ringbuf_space (const ringbuf *r);

size_t ringbuf_write (ringbuf *r, const void *data, size_t len);
size_t ringbuf_read (ringbuf *r, void *out, size_t len);

void ringbuf_peek (const ringbuf *r, void **ptr1, size_t *len1, void **ptr2, size_t *len2);
int ringbuf_peek_iov (const ringbuf *r, struct iovec iov[2]);
void ringbuf_consume (ringbuf *r, size_t n);

#endif
