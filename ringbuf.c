#include "ringbuf.h"

ringbuf *
ringbuf_new (size_t capacity)
{
    ringbuf *r = malloc (sizeof *r);
    if (!r || capacity == 0) return NULL;
    uint8_t *d = malloc (capacity);
    if (!d) return NULL;
    r->data = d;
    r->size = capacity;
    r->head = r->tail = r->count = 0;
    return r;
}

void
ringbuf_free (ringbuf *r)
{
    if (!r) return;
    free (r->data);
    r->data = NULL;
    r->size = r->head = r->tail = r->count = 0;
    free (r);
}

size_t
ringbuf_len (const ringbuf *r)
{
    return r->count;
}

size_t
ringbuf_space (const ringbuf *r)
{
    return r->size - r->count;
}

size_t
ringbuf_write (ringbuf *r, const void *data, size_t len)
{
    if (len == 0) return 0;
    size_t space = ringbuf_space (r);
    if (len > space) len = space;
    if (len == 0) return 0;

    size_t first = r->size - r->head;
    if (first > len) first = len;
    memcpy (r->data + r->head, data, first);
    r->head += first;
    if (r->head == r->size) r->head = 0;

    size_t rest = len - first;
    if (rest)
    {
        memcpy (r->data + r->head, (const uint8_t *)data + first, rest);
        r->head += rest;
        if (r->head == r->size) r->head = 0;
    }

    r->count += len;
    return len;
}

size_t
ringbuf_read (ringbuf *r, void *out, size_t len)
{
    if (len == 0) return 0;
    size_t avail = ringbuf_len (r);
    if (len > avail) len = avail;
    if (len == 0) return 0;

    size_t first = r->size - r->tail;
    if (first > len) first = len;
    memcpy (out, r->data + r->tail, first);
    r->tail += first;
    if (r->tail == r->size) r->tail = 0;

    size_t rest = len - first;
    if (rest)
    {
        memcpy ((uint8_t *)out + first, r->data + r->tail, rest);
        r->tail += rest;
        if (r->tail == r->size) r->tail = 0;
    }

    r->count -= len;
    return len;
}

void
ringbuf_peek (const ringbuf *r, void **ptr1, size_t *len1, void **ptr2, size_t *len2)
{
    if (ptr1) *ptr1 = NULL;
    if (len1) *len1 = 0;
    if (ptr2) *ptr2 = NULL;
    if (len2) *len2 = 0;

    size_t avail = ringbuf_len (r);
    if (avail == 0) return;

    size_t first = r->size - r->tail;
    if (first > avail) first = avail;

    if (ptr1) *ptr1 = (void *)(r->data + r->tail);
    if (len1) *len1 = first;

    size_t rest = avail - first;
    if (rest > 0)
    {
        if (ptr2) *ptr2 = (void *)r->data;
        if (len2) *len2 = rest;
    }
}

int
ringbuf_peek_iov (const ringbuf *r, struct iovec iov[2])
{
    void *p1 = NULL, *p2 = NULL;
    size_t l1 = 0, l2 = 0;
    ringbuf_peek (r, &p1, &l1, &p2, &l2);
    int n = 0;
    if (l1)
    {
        iov[n].iov_base = p1;
        iov[n].iov_len = l1;
        ++n;
    }
    if (l2)
    {
        iov[n].iov_base = p2;
        iov[n].iov_len = l2;
        ++n;
    }
    return n;
}

void
ringbuf_consume (ringbuf *r, size_t n)
{
    if (n == 0) return;
    if (n >= r->count)
    {
        r->head = r->tail = r->count = 0;
        return;
    }
    r->tail += n;
    if (r->tail >= r->size) r->tail -= r->size;
    r->count -= n;
}