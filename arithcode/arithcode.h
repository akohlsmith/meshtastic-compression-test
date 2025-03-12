#ifndef _ARITHCODE_H_
#define _ARITHCODE_H_

#include <stdint.h>
#include <stdlib.h>

/* since I'm using fixed-size CDFs, this sets the upper limit to the number of symbols */
#define CDF_MAX_SYMB	(384)

typedef uint8_t		u8;
typedef uint32_t	u32;
typedef uint64_t	u64;
typedef float		real;


/*
 * encode
 * ------
 *  <out>   <*out> may be NULL or point to a preallocated buffer with <*nout> 
 *          elements.  The buffer may be reallocated if it's not big   enough,
 *          in which case the new size and adress are output via <*out> and
 *          <*nout>.
 *  <nsym>  The number of symbols.
 *  <cdf>   The cumulative distribution of the symbols 0..<nsym>-1.
 *          It should be specified as a sequence of <nsym>+1 real numbers.
 *          cdf[0] shold be 0.0, and cdf[nsym] should be 1.0.  The sequence
 *          should be monotonically increasing.  No symbol should have
 *          probability less than the resolution of the coder, which depends on
 *          the output bit depth. See the source for details.  As a rule of
 *          thumb, probabilities should be greater than 1/2^(32-bitsof(output
 *          symbol)).
 * 
 *          Note that this doesn't have to be the _actual_ probability of the
 *          symbol.  Differences will result in a slightly less efficient
 *          compression.
 *                                                                      
 * decode                                                               
 * ------                                                               
 *  <out>   output buffer, should be pre-allocated                     
 *  <nout>  must be less than or equal to the actual message size      
 *  <in>    the bit string returned in encode's <out.d>                
 *  <c>     the CDF over output symbols                                
 *  <nsym>  the number of output symbols                               
 */

real *cdf_build(real *cdf, size_t *nsym, u8 *s, size_t ns);

int encode_u8_u8(void **out, size_t *nout, void *in, size_t nin, real *cdf, size_t nsym);
int decode_u8_u8(void **out, size_t *nout, void *in, size_t nin, real *cdf, size_t nsym);

#endif /* _ARITHCODE_H_ */
