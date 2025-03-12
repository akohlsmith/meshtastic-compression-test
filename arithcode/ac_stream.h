#ifndef _AC_STREAM_H_
#define _AC_STREAM_H_

#include <stdint.h>
#include <stdlib.h>

/*
 * Bit Stream
 * - will realloc to resize if necessary
 * - push appends to end
 * - pop removes from front
 * - models a FIFO stream
 * - push/pop are not meant to work together.  That is, can't simultaneously
 *   encode and decode on the same stream; Can't interleave push/pop.
 *
 * - carry is somewhat specific to arithmetic coding, but seems like a stream
 *   op, so it's included here.  Alternatively, I could add an operation to
 *   rewind the stream a bit so that the carry op could be implemented
 *   elsewhere.
 */

typedef struct _stream_t {
	size_t nbytes;	/* capacity */
	size_t ibyte;	/* current byte */
	size_t ibit; 	/* current bit   (for u8 stream this is always 0) */
	uint8_t mask;	/* bit is set in the position of the last write */
	uint8_t *d;	/* data */
	int own;	/* ownship flag: should this object be responsible for freeing d [??:used] */
} stream_t;

/*
 * Attach
 * ------
 * if <d> is NULL, will allocate some space.
 * otherwise, free's the internal buffer (if there is one),
 *            and uses <d> instead.
 * <n> is the byte size of <d>
 *
 * Detach
 * ------
 * returns the pointer to the streams buffer and it's size via <d> and <n>.
 * The stream, disowns (but does not free) the buffer, returning the stream
 * to an empty state.
 */
int attach(stream_t *s, void *d, size_t n);
void detach(stream_t *s, void **d, size_t *n);

int push_u8(stream_t *s, uint8_t v);
uint8_t pop_u8(stream_t *s);
void carry_u8(stream_t* s);

#endif /* _AC_STREAM_H_ */
