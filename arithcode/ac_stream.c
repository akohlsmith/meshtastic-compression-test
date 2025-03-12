#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ac_stream.h"

typedef uint8_t u8;

#define SAFE_FREE(e) if (e) { free(e); (e) = NULL; }

/* returns 0 if we have a buffer, -1 otherwise */
static int maybe_init(stream_t *self)
{
	int ret;

	ret = -1;
	if (! self->d) {
		memset(self, 0, sizeof(*self));
		self->nbytes = 256;

		if ((self->d = malloc(self->nbytes))) {
			self->own = 1;
			ret = 0;
		}

	/* self->d isn't NULL, there's already a buffer there so we're good */
	} else {
		ret = 0;
	}

	return ret;
}

/* returns 0 if we have a buffer, -1 otherwise */
int attach(stream_t *self, void *d, size_t n)
{
	if (self->own) {
		SAFE_FREE(self->d);
	}

	self->d = (u8 *)d;
	self->own = 0;
	self->nbytes = n;
	return maybe_init(self);
}

void detach(stream_t *self, void **d, size_t *n)
{
	if(d) *d = self->d;
	if(n) *n = self->ibyte;
	memset(self, 0, sizeof(*self));
}

/* returns 0 if we (still) have a buffer, -1 if we couldn't (re)alloc */
static int maybe_resize(stream_t *s)
{
	int ret;

	if (s->ibyte >= s->nbytes) {
#if 0
		s->nbytes = (1.2 * s->ibyte + 50);
		if ((s->d = realloc(s->d, s->nbytes))) {
			ret = 0;
		} else {
			ret = -1;
		}
#else
		ret = -1;
#endif
	} else {
		ret = 0;
	}

	return ret;
}

/* returns 0 if the push was successful, negative otherwise */
int push_u8(stream_t *self, u8 v)
{
	*(u8 *)(self->d + self->ibyte) = v;
	self->ibyte += sizeof(v);
	return maybe_resize(self);
}

u8 pop_u8(stream_t *self)
{
	u8 v;
	if (self->ibyte >= self->nbytes) {
		return 0;
	}

	v = *(u8 *)(self->d + self->ibyte);
	self->ibyte += sizeof(u8);
	return v;
}


static const u8 max_u8 = (u8)(-1);

void carry_u8(stream_t *self)
{
	size_t n = (self->ibyte - 1) / sizeof(u8);
	while (((u8 *)(self->d))[n] == max_u8) {
		((u8 *)(self->d))[n--] = 0;
	};

	((u8 *)(self->d))[n]++;
}
