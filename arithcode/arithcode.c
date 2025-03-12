/*
 * Arithmetic coding implementation
 * Implementation after Amir Said's Algorithm 22-29[1].
 *
 * Following Amir's notation,
 * D is the number of symbols in the output alphabet.
 * P is the number of output symbols in the active block (see Eq 2.3 in [1]).
 * Need 2P for multiplication.
 *
 * So, have 2P*bitsof(D) = 64 (widest integer type I'll have access to)
 *
 * The smallest codable probabililty is p = D/D^P = D^(1-P).  Want to minimize
 * p wrt constraint. Enumerating the possibilities:
 *
 *      bitsof(D)     P                D^(1-P)
 *      --------      --               -------
 *         1          32                2^-32
 *         8          4    (2^8)^(-3) = 2^-24
 *         16         2                 2^-16
 *         32         1                 1
 *
 * One can see there's a efficiency verses precision trade off here (higher D
 * means less renormalizing and so better efficiency).
 *
 * Notes
 * - Need to test more!
 * - u1,u4,u16 encoding/decoding not tested
 * - random sequences etc...
 * - empty stream?
 * - streams that generte big carries
 *
 * - might be nice to add an interface that will encode chunks.  That is,
 * you feed it symbols one at a time and every once in a while, when
 * bits get settled, it outputs some bytes.
 *
 * For streaming encoders, this would mean the intermediate buffer
 * wouldn't have to be quite as big, although worst case that
 * buffer is the size of the entire output message.
 *
 * - A check-symbol could be encoded in a manner similar to the END-OF-MESSAGE
 * symbol.  For example, code the symbol every 2^x symbols and assign it a
 * probability of 2^-x.  If the decoder doesn't recieve one of these at the
 * expected interval, then there was some error.  For short messages,
 * it wouldn't require the insertion of such a symbol; only failure to show
 * up after N symbols would indicate an error.  It would take space
 * on the interval, and so would have a negative impact on compression and
 * minimum probability.
 *
 * References
 * [1]:  Said, A. "Introduction to Arithmetic Coding - Theory and Practice."
 * Hewlett Packard Laboratories Report: 2004-2076.
 * www.hpl.hp.com/techreports/2004/HPL-2004-76.pdf
 */

/*
 * Arithmetic Coding
 *
 * Contents
 * - Example
 * - Features
 * - History
 * - CDFs
 * - Encoding
 * - Decoding
 *
 * Example
 * void encode()
 * {
 *	unsigned char *input, *output=0;	// input and output buffer
 *	size_t countof_input, countof_output;	// number of symbols in input and output buffer
 *	float *cdf=0;
 *	size_t nsym;				// number of symbols in the input alphabet
 *
 *	// somehow load the data into input array
 *	cdf_build(&cdf, &nsym, input, countof_input);
 *	encode_u1_u8(                         // encode unsigned chars to a string of bits (1 bit per output symbol)
 * 		(void**)&out, &countof_output, input, countof_input, cdf, nsym);
 *
 * 	// do something with the output
 * 	free(out);
 * 	free(cdf);
 * }
 *
 * Features
 * - Encoding/decoding to/from variable symbol alphabets.
 * - Implicit use of a STOP symbol means that you don't need to know the number of symbols in the decoded message in order to decode something.
 * - Can encoded messages stored as signed or unsigned chars, shorts, longs or long longs.
 *   Keep in mind, however, that the number of encodable symbols may be limiting.  You can't encode 2^64 different integers, sorry.
 * - Can encode to streams of variable symbol width; either 1,4,8, or 16 bits.  There is are two tradeoffs here.
 *     - Smaller bit-width (e.g. 1) give better compression than larger bit-width (e.g. 16), but compression is slower (I think?).
 *     - The implimentation puts a limit on the smallest probability of an encoded symbol.  Smaller bit-width (e.g. 1) can accomidate a larger
 *       range of probabilities than large bit-width (e.g. 16).
 *
 * Todo: Markov chain adaptive encoding/decoding.
 *
 * History History and Caveats
 * I wrote this in order to learn about arithmetic coding.  The end goal was to get to the point where I had an adaptive
 * encoder/decoder that could compress markov chains.  I got a little distracted along the way by trying to encode to
 * a variable symbol alphabet.  Variable symbol encoding alphabets are fun because you can encode to non-whitespace ASCII
 * characters (94 symbols) and that means the encoded message can be embeded in a text file format.
 * Arithmetic coding is a computationally expensive coding method.  I've done nothing to address this problem; I've probably
 * made it worse.
 *
 * To learn more about arithmetic coding see this excelent reference:
 * Said, A. “Introduction to Arithmetic Coding - Theory and Practice.”
 * 		 Hewlett Packard Laboratories Report: 2004–2076.
 * 		 www.hpl.hp.com/techreports/2004/HPL-2004-76.pdf
 *
 * CDFs Cumulative Distribution Functions (CDFs)
 * All the encoding and decoding functions require knowledge of the probability of observing input symbols.  This probability is
 * specified as a CDF of a particular form.
 *
 * Namely:
 *   1. cdf[0] must be zero (0.0).
 *   2. The probability density for symbol i should be cdf[i+1]-cdf[i].
 *
 * This implies, for an alphabet with M symbols:
 *   - cdf has M+1 elements.
 *   - cdf[M] is 1.0 (within floating point precision)
 *
 * see cdf_build() for an example of how to build a CDF from a given input message.
 *
 * Encoding Encoding Functions
 * Encoding functions all have the same form:
 *     encode_<TDst>_<TSrc>(void **out, size_t *nout, uint8_t *in, size_t nin, float *cdf, size_t nsym);
 *
 * where TDst and TSrc are the output and input types, respectively.  The output buffer, *out,
 * can be an heap-allocated buffer; it's capacity in bytes should be passed as *nout.  If *out is
 * not large enough (or NULL), it will be reallocated.  The new capacity will be returned in *nout.
 *
 * Decoding Decoding functions
 *
 * Decoding functions all have the same form:
 *
 * decode_<TDst>_<TSrc>(void **out, size_t *nout, uint8_t *in, size_t nin, float *cdf, size_t nsym);
 *
 * where TDst and TSrc are the output and input types, respectively.  The output buffer, *out,
 * can be an heap-allocated buffer; it's capacity in bytes should be passed as *nout.  If *out is
 * not large enough (or NULL), it will be reallocated.  The new capacity will be returned in *nout.
 *
 * This is basically identical to the encode functions, but here "output" refers to the decoded message
 * and "input" refers to an encoded message.
 *
 * Nathan Clack <https://github.com/nclack>
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "arithcode.h"
#include "ac_stream.h"

#define SAFE_FREE(e) if (e) { free(e); (e) = NULL; }

/*
 * Encoder/Decoder state
 * Used internally. Not externally visible.
 */
typedef struct _state_t {
	u64 b,			/* Beginning of the current interval. */
	    l,			/* Length of the current interval. */
	    v;			/* Current value. */

	stream_t d;		/* The attached data stream. */
	size_t nsym;		/* The number of symbols in the input alphabet. */
	u64 D,			/* The number of symbols in the output alphabet. */
	    shift,		/* A utility constant.  log2(D^P) - need 2P to fit in a register for multiplies. */
	    mask,		/* Masks the live bits (can't remember exactly?) */
	    lowl;		/* The minimum length of an encodable interval. */
	u64 cdf[CDF_MAX_SYMB];	/* The cdf associated with the input alphabet.  Must be an array of N+1 symbols. */
} state_t;

/* A helper function that initializes the parts of the \ref state_t structure that do not depend on stream type. */
static int init_common(state_t *state, u8 *buf, size_t nbuf, real *cdf, size_t nsym)
{
	const size_t cdf_len = nsym * sizeof(state->cdf[0]);
	state->l = (1ULL << state->shift) - 1;	/* e.g. 2^32-1 for u64 */
	state->mask = state->l;			/* for modding a u64 to u32 with & */

	nsym++;					/* add end symbol */
	if (nsym < CDF_MAX_SYMB /*(state->cdf = malloc(cdf_len))*/) {
		size_t i;

		real s = state->l - state->D;	/* scale to D^P range and adjust for end symbol */
		for (i = 0; i < nsym - 1; i++) {
			state->cdf[i] = s * cdf[i];
		}

		state->cdf[i] = s;
		state->nsym = nsym;

		attach(&state->d, buf, nbuf);
		return 0;
	}

	printf("%s: number of symbols %zd too large for maximum %zd\n", __func__, nsym, CDF_MAX_SYMB);
	return -1;
}

static int init_u8(state_t *state, u8 *buf, size_t nbuf, real *cdf, size_t nsym)
{
	memset(state, 0, sizeof(*state));
	state->D = 1ULL << 8;
	state->shift = 32;		/* log2(D^P) - need 2P to fit in a register for multiplies */
	state->lowl = 1ULL << 24;	/* 2^(shift - log2(D)) */
	return init_common(state, buf, nbuf, cdf, nsym);
}

static void free_internal(state_t *state)
{
	//SAFE_FREE(state->cdf);
}

/* Array maximum. returns the maximum value in a u8 array, s, with n elements. */
static u8 maximum(u8 *s, size_t n)
{
	u8 max = 0;
	while (n--) {
		max = (s[n] > max) ? s[n] : max;
	}

	return max;
}

/*
 * Build a cumulative distribution function (CDF) from an input u32 array.
 *
 * As provided, this really just serves as a reference implementation showing how
 * the CDF should be computed.
 *
 * Namely:
 * 	1. cdf[0] must be zero (0.0).
 * 	2. The probability density for symbol i should be cdf[i+1] - cdf[i].
 *
 * This implies:
 *   - cdf has M+1 elements.
 *   - cdf[(*M) + 1] == 1.0 (within floating point precision)
 *
 * cdf the cumulative distribution function computed over s. If *cdf is not null, will realloc() if necessary.
 * M   The number of symbols in the array s.
 * s   The input message formated as a u8 array of N elements.
 * N   The number of elements in the array s.
 *
*/
real *cdf_build(real *cdf, size_t *M, u8 *s, size_t N)
{
	size_t cdf_len;

	*M = maximum(s, N) + 1;
	cdf_len = sizeof(*cdf) * (M[0] + 1);

	if ((M[0] + 1) > CDF_MAX_SYMB) {
		printf("%s: too many symbols (%zd > %zd)\n", __func__, M[0] + 1, CDF_MAX_SYMB);
		return NULL;
	}

	if (cdf) {//(cdf = malloc(cdf_len))) {
		size_t i;
		memset(cdf, 0, cdf_len);

		for (i = 0;      i < N;    i++) cdf[s[i]]++;		/* histogram */
		for (i = 0;      i < M[0]; i++) cdf[i] /= (real)N;	/* norm */
		for (i = 1;      i < M[0]; i++) cdf[i] += cdf[i - 1];	/* cumsum */
		for (i = M[0]+1; i > 0;    i--) cdf[i] = cdf[i-1];	/* move */
		cdf[0] = 0.0;

	} else {
		printf("%s: could not allocate %zd bytes for CDF\n", __func__, cdf_len);
	}

	return cdf;
}


/* encoder */

#define B         (state->b)
#define L         (state->l)
#define C         (state->cdf)
#define SHIFT     (state->shift)
#define NSYM      (state->nsym)
#define MASK      (state->mask)
#define STREAM    (&(state->d))
#define DATA      (state->d.d)
#define OFLOW     (STREAM->overflow)
#define bitsofD   (8)
#define D         (1ULL<<bitsofD)
#define LOWL      (state->lowl)

static int update_u8(u64 s, state_t *state)
{
	u64 a, x, y;

	y = L;			/* End of interval */
	if (s != (NSYM - 1)) {	/* is not last symbol */
		y = (y * C[s + 1]) >> SHIFT;
	}

	a = B;
	x = (L * C[s]) >> SHIFT;
	B = (B + x) & MASK;
	L = y - x;

	if (L > 0) {
		if (a > B) {
			carry_u8(STREAM);
		}

		return 0;
	}

	return -1;
}

static int erenorm_u8(state_t *state)
{
	int ret;
	const int s = SHIFT - bitsofD;

	ret = 0;
	while (ret == 0 && L < LOWL) {
		ret = push_u8(STREAM, B >> s);
		L = (L << bitsofD) & MASK;
		B = (B << bitsofD) & MASK;
	};

	return ret;
}

static int eselect_u8(state_t *state)
{
	u64 a;

	a = B;

	/* D^(P-1)/2: (2^8)^(4-1)/2 = 2^24/2 = 2^23 = 2^(32-8-1) */
	B = (B + (1ULL << (SHIFT - bitsofD - 1))) & MASK;

	L = (1ULL << (SHIFT - 2 * bitsofD)) - 1;	/* requires P > 2 */
	if (a > B) {
		carry_u8(STREAM);
	}

	/* output last 2 symbols */
	return erenorm_u8(state);
}

static int estep_u8(state_t *state, u64 s)
{
	int ret;
	if ((ret = update_u8(s, state)) == 0) {
		if (L < LOWL) {
			ret = erenorm_u8(state);
		}
	}

	return ret;
}

/* returns 0 for successful encode, negative otherwise */
int encode_u8_u8(void **out, size_t *nout, void *in, size_t nin, real *cdf, size_t nsym)
{
	size_t i;
	static state_t s;
	int ret;

	if ((ret = init_u8(&s, *out, *nout, cdf, nsym)) == 0) {
		for (i = 0; ret == 0 && i < nin; i++) {
			ret = estep_u8(&s, ((u8 *)in)[i]);
		}

		if ((ret = estep_u8(&s, s.nsym - 1)) == 0) {
			ret = eselect_u8(&s);
		}
		detach(&s.d, out, nout);
		free_internal(&s);
	}

	return ret;
}


/* decode */
static u64 dselect(state_t *state, u64 *v, int *isend)
{
	u64 s, n, x, y;

	s = 0;
	n = NSYM;
	x = 0;
	y = L;
	while ((n - s) > 1UL) {		/* bisection search */
		u32 m = (s + n) >> 1;
		u64 z = (L * C[m]) >> SHIFT;

		if (z > *v) {
			n = m;
			y = z;

		} else {
			s = m;
			x = z;
		}
	};

	*v -= x;
	L = y - x;

	if (s == (NSYM - 1)) {
		*isend = 1;
	}

	return s;
}

static void drenorm_u8(state_t *state, u64 *v)
{
	while (L < LOWL) {
		*v = ((*v << bitsofD) & MASK) + pop_u8(STREAM);
		L = (L << bitsofD) & MASK;
	};
}

static void dprime_u8(state_t *state, u64* v)
{
	size_t i;
	*v = 0;
	for (i = bitsofD; i <= SHIFT; i += bitsofD) {
		/*(2^8)^(P-n) = 2^(8*(P-n))*/
		*v += (1ULL << (SHIFT - i)) * pop_u8(STREAM);
	}
}

static u64 dstep_u8(state_t *state, u64 *v, int *isend)
{
	u64 s = dselect(state, v, isend);
	if (L < LOWL) {
		drenorm_u8(state, v);
	}

	return s;
}

int decode_u8_u8(void **out, size_t *nout, void *in, size_t nin, real *cdf, size_t nsym)
{
	state_t s;
	stream_t d = {0};
	u64 v, x;
	size_t i = 0;
	int isend = 0;
	int ret;

	attach(&d, *out, *nout * sizeof(u8));
	if ((ret = init_u8(&s, in, nin, cdf, nsym)) == 0) {
		dprime_u8(&s, &v);
		x = dstep_u8(&s, &v, &isend);
		while (! isend) {
			push_u8(&d, x);
			x = dstep_u8(&s, &v, &isend);
		}

		free_internal(&s);
		detach(&d, (void**)out, nout);
		*nout /= sizeof(u8);
	}

	return ret;
}
