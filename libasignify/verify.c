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
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "asignify.h"
#include "asignify_internal.h"
#include "khash.h"
#include "kvec.h"

struct asignify_verify_file {
	char *fname;
	enum asignify_digest_type digest_type;
	unsigned char *digest;
	size_t digest_len;
};

KHASH_INIT(asignify_verify_hnode, const char *, struct asignify_verify_file, 1,
	kh_str_hash_func, kh_str_hash_equal);

struct asignify_verify_ctx {
	struct asignify_pubkey *pk;
	khash_t(asignify_verify_hnode) *files;
	const char *error;
};

static char *
asignify_verify_load_sig(struct asignify_verify_ctx *ctx, FILE *f, size_t *len)
{
	const size_t maxlen = 1 << 30;
	struct stat st;
	int r;
#if BUFSIZ >= 2048
	unsigned char buf[BUFSIZ];
#else
	/* BUFSIZ is insanely small */
	unsigned char buf[4096];
#endif
	kvec_t(char) res;

	if (ctx == NULL || f == NULL || fstat(fileno(f), &st) == -1) {
		return (NULL);
	}

	if (S_ISREG(st.st_mode) && st.st_size > maxlen) {
		ctx->error = xerr_string(ASIGNIFY_ERROR_FILE);
		return (NULL);
	}

	kv_init(res);

	while ((r = fread(buf, 1, sizeof(buf), f)) > 0) {
		kv_push_a(char, res, buf, r);
	}

	*len = kv_size(res);

	return (res.a);
}

asignify_verify_t*
asignify_verify_init(void)
{
	asignify_verify_t *nctx;

	nctx = xmalloc0(sizeof(*nctx));

	return (nctx);
}


bool
asignify_verify_load_pubkey(asignify_verify_t *ctx, const char *pubf)
{
	FILE *f;
	bool ret = false;

	if (ctx == NULL) {
		return (false);
	}

	f = xfopen(pubf, "r");
	if (f == NULL) {
		ctx->error = xerr_string(ASIGNIFY_ERROR_FILE);
	}
	else {
		ctx->pk = asignify_pubkey_load(f);
		if (ctx->pk == NULL) {
			ctx->error = xerr_string(ASIGNIFY_ERROR_FORMAT);
		}
		else {
			ret = true;
		}
	}

	return (ret);
}

bool
asignify_verify_load_signature(asignify_verify_t *ctx, const char *sigf)
{
	struct asignify_signature *sig;
	char *data;
	size_t dlen;
	FILE *f;

	if (ctx == NULL || ctx->pk == NULL) {
		if (ctx) {
			ctx->error = xerr_string(ASIGNIFY_ERROR_MISUSE);
		}
		return (false);
	}

	f = xfopen(sigf, "r");
	if (f == NULL) {
		ctx->error = xerr_string(ASIGNIFY_ERROR_FILE);
	}
	else {
		sig = asignify_signature_load(f);
		if (ctx->pk == NULL) {
			ctx->error = xerr_string(ASIGNIFY_ERROR_FORMAT);
		}
		else {
			data = asignify_verify_load_sig(ctx, f, &dlen);
			if (data == NULL || dlen == 0) {
				return (false);
			}

			if (!asignify_pubkey_check_signature(ctx->pk, sig, data, dlen)) {
				return (false);
			}

			/* We are now safe to parse digests */
		}
	}

	return (false);
}

bool
asignify_verify_file(asignify_verify_t *ctx, const char *checkf)
{
	return (false);
}


const char*
asignify_verify_get_error(asignify_verify_t *ctx)
{
	if (ctx == NULL) {
		return (xerr_string(ASIGNIFY_ERROR_MISUSE));
	}

	return (ctx->error);
}

void
asignify_verify_free(asignify_verify_t *ctx)
{
	if (ctx) {
		asignify_pubkey_free(ctx->pk);
		kh_destroy(asignify_verify_hnode, ctx->files);
	}
}
