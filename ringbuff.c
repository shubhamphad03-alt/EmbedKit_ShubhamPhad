/*
 * ringbuff.c
 *
 * Single-file C99 implementation of an 8-byte circular (ring) buffer
 * for uint8_t data. Demonstrates writes, reads, wrap-around behavior,
 * and proper status codes. Written to embedded firmware standards.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/* Configuration constants --------------------------------------------------*/
#define BUFFER_SIZE 8u
/* BUFFER_MASK is used to wrap indexes using bitwise AND instead of modulo.
 * This is faster on MCUs without hardware division and requires BUFFER_SIZE
 * to be a power of two. For example, (index + 1) & BUFFER_MASK is equivalent
 * to (index + 1) % BUFFER_SIZE when BUFFER_SIZE is a power of two.
 */
#define BUFFER_MASK (BUFFER_SIZE - 1u)

/* Status codes */
#define RINGBUF_OK    0
#define RINGBUF_FULL  (-1)
#define RINGBUF_EMPTY (-2)

/* Ring buffer data structure ----------------------------------------------*/
typedef struct {
    uint8_t buffer[BUFFER_SIZE]; /* storage */
    uint8_t head;                /* next write index */
    uint8_t tail;                /* next read index */
    uint8_t count;               /* number of bytes currently stored */
} ringbuf_t;

/* Initialize the ring buffer to an empty state. Clears storage for
 * deterministic behavior in tests and debugging.
 */
void ringbuf_init(ringbuf_t *rb)
{
    if (rb == NULL) {
        return;
    }
    memset(rb->buffer, 0, sizeof(rb->buffer));
    rb->head = 0u;
    rb->tail = 0u;
    rb->count = 0u;
}

/* Return non-zero if buffer is full */
int ringbuf_is_full(const ringbuf_t *rb)
{
    return (rb != NULL) && (rb->count == (uint8_t)BUFFER_SIZE);
}

/* Return non-zero if buffer is empty */
int ringbuf_is_empty(const ringbuf_t *rb)
{
    return (rb != NULL) && (rb->count == 0u);
}

/* Return current count of stored bytes */
uint8_t ringbuf_count(const ringbuf_t *rb)
{
    if (rb == NULL) {
        return 0u;
    }
    return rb->count;
}

/* Write a byte into the ring buffer. Returns RINGBUF_OK on success or
 * RINGBUF_FULL if the buffer has no free space. This implementation never
 * overwrites unread data.
 */
int ringbuf_write(ringbuf_t *rb, uint8_t data)
{
    if (rb == NULL) {
        return RINGBUF_FULL;
    }

    if (ringbuf_is_full(rb)) {
        return RINGBUF_FULL;
    }

    rb->buffer[rb->head] = data;
    /* Use bitwise AND to wrap index quickly (requires power-of-two size). */
    rb->head = (uint8_t)((rb->head + 1u) & BUFFER_MASK);
    rb->count = (uint8_t)(rb->count + 1u);

    return RINGBUF_OK;
}

/* Read a byte from the ring buffer. Returns RINGBUF_OK on success and
 * stores the byte at *data. Returns RINGBUF_EMPTY if the buffer contains
 * no data.
 */
int ringbuf_read(ringbuf_t *rb, uint8_t *data)
{
    if (rb == NULL || data == NULL) {
        return RINGBUF_EMPTY;
    }

    if (ringbuf_is_empty(rb)) {
        return RINGBUF_EMPTY;
    }

    *data = rb->buffer[rb->tail];
    rb->tail = (uint8_t)((rb->tail + 1u) & BUFFER_MASK);
    rb->count = (uint8_t)(rb->count - 1u);

    return RINGBUF_OK;
}

/* Main demonstration ------------------------------------------------------*/
int main(void)
{
    ringbuf_t rb;
    ringbuf_init(&rb);

    /* 1) Write 0x41..0x48 and print count after every write. */
    for (uint8_t v = 0x41u; v <= 0x48u; ++v) {
        int res = ringbuf_write(&rb, v);
        if (res == RINGBUF_OK) {
            uint8_t c = ringbuf_count(&rb);
            if (c == BUFFER_SIZE) {
                printf("[WRITE] 0x%02X -> OK (count=%u) FULL\n", v, (unsigned)c);
            } else {
                printf("[WRITE] 0x%02X -> OK (count=%u)\n", v, (unsigned)c);
            }
        } else {
            printf("[WRITE] 0x%02X -> FAIL (buffer full)\n", v);
        }
    }

    /* Confirm buffer is full and count == 8 (implicit in output above). */

    /* 2) Attempt to write 0x99 (should fail). */
    {
        uint8_t val = 0x99u;
        int res = ringbuf_write(&rb, val);
        if (res != RINGBUF_OK) {
            printf("[WRITE] 0x%02X -> FAIL (buffer full)\n", val);
        } else {
            printf("[WRITE] 0x%02X -> OK (count=%u)\n", val, (unsigned)ringbuf_count(&rb));
        }
    }

    /* 3) Read 3 bytes and print them. */
    for (int i = 0; i < 3; ++i) {
        uint8_t out;
        int res = ringbuf_read(&rb, &out);
        if (res == RINGBUF_OK) {
            printf("[READ] -> 0x%02X (count=%u)\n", out, (unsigned)ringbuf_count(&rb));
        } else {
            printf("[READ] (empty) -> FAIL (buffer empty)\n");
        }
    }

    /* After reading 3 bytes, count should be 5. */

    /* 4) Write 0x49, 0x4A, 0x4B to demonstrate wrap-around. */
    for (uint8_t v = 0x49u; v <= 0x4Bu; ++v) {
        int res = ringbuf_write(&rb, v);
        if (res == RINGBUF_OK) {
            uint8_t c = ringbuf_count(&rb);
            if (c == BUFFER_SIZE) {
                printf("[WRITE] 0x%02X -> OK (count=%u) FULL\n", v, (unsigned)c);
            } else {
                printf("[WRITE] 0x%02X -> OK (count=%u)\n", v, (unsigned)c);
            }
        } else {
            printf("[WRITE] 0x%02X -> FAIL (buffer full)\n", v);
        }
    }

    /* 5) Read all remaining bytes and print them in FIFO order. */
    while (!ringbuf_is_empty(&rb)) {
        uint8_t out;
        int res = ringbuf_read(&rb, &out);
        if (res == RINGBUF_OK) {
            printf("[READ] -> 0x%02X (count=%u)\n", out, (unsigned)ringbuf_count(&rb));
        } else {
            printf("[READ] (empty) -> FAIL (buffer empty)\n");
            break;
        }
    }

    /* 6) Confirm buffer is empty. */

    /* 7) Attempt one more read (should fail). */
    {
        uint8_t out;
        int res = ringbuf_read(&rb, &out);
        if (res != RINGBUF_OK) {
            printf("[READ] (empty) -> FAIL (buffer empty)\n");
        } else {
            printf("[READ] -> 0x%02X (count=%u)\n", out, (unsigned)ringbuf_count(&rb));
        }
    }

    return 0;
}
