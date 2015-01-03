/* Copyright (c) 2015, Vsevolod Stakhov
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *       * Redistributions of source code must retain the above copyright
 *         notice, this list of conditions and the following disclaimer.
 *       * Redistributions in binary form must reproduce the above copyright
 *         notice, this list of conditions and the following disclaimer in the
 *         documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_OPENSSL
#include <openssl/rand.h>
#endif
#ifdef HAVE_BSD_STDLIB_H
#include <bsd/stdlib.h>
#endif

#include "asignify_internal.h"

#ifdef HAVE_WEAK_SYMBOLS
__attribute__((weak)) void
_dummy_symbol_to_prevent_lto(void * const pnt, const size_t len)
{
	(void) pnt;
	(void) len;
}
#endif

void explicit_memzero(void * const pnt, const size_t len)
{
#if defined(HAVE_MEMSET_S)
	if (memset_s(pnt, (rsize_t) len, 0, (rsize_t) len) != 0) {
		abort();
	}
#elif defined(HAVE_EXPLICIT_BZERO)
	explicit_bzero(pnt, len);
#elif HAVE_WEAK_SYMBOLS
	memset(pnt, 0, len);
	_dummy_symbol_to_prevent_lto(pnt, len);
#else
	volatile unsigned char *pnt_ = (volatile unsigned char *) pnt;
	size_t i = (size_t) 0U;
	while (i < len) {
		pnt_[i++] = 0U;
	}
#endif
}


void
randombytes(unsigned char *buf, uint64_t len)
{
#ifdef HAVE_ARC4RANDOM
	arc4random_buf(buf, len);
#elif defined(HAVE_OPENSSL)
	if (RAND_bytes(buf, len) != 1) {
		abort();
	}
#else
# error No random numbers can be generated on your system
#endif
}