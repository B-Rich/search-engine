#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include "codec.h"

struct codec *codec_new(enum codec_method method, void* args)
{
	size_t args_sz;
	struct codec *ret = malloc(sizeof(struct codec));

	switch (method) {
	case CODEC_FOR:
	case CODEC_FOR_DELTA:
		args_sz = sizeof(struct for_delta_args);
		break;
	case CODEC_PLAIN:
		args_sz = 0;
		break;
	case CODEC_GZ:
		args_sz = 0;
		break;
	default:
		assert(0);
	}

	ret->method = method;
	ret->args = malloc(args_sz);

	if(args)
		memcpy(ret->args, args, args_sz);
	else
		memset(ret->args, 0, args_sz);

	return ret;
}

void codec_free(struct codec* codec)
{
	free(codec->args);
	free(codec);
}

char *codec_method_str(enum codec_method method)
{
	static char ret[4096];
	switch (method) {
	case CODEC_FOR:
		strcpy(ret, "Frame of Reference codec");
		break;
	case CODEC_FOR_DELTA:
		strcpy(ret, "Frame of Reference delta codec");
		break;
	case CODEC_GZ:
		strcpy(ret, "GNU zip codec");
		break;
	case CODEC_PLAIN:
		strcpy(ret, "No codec (plain)");
		break;
	default:
		strcpy(ret, "Unknown codec");
		break;
	}

	return ret;
}

static size_t
dummpy_copy(const uint32_t *in, size_t len, void *out)
{
	size_t n_bytes = len << 2;
	memcpy(out, in, n_bytes);
	return n_bytes;
}

/*
 * Compress an uint32_t integer array `in' of length `len', using the
 * algorithm and its parameters specified from `codec'. It writes the
 * compressed content to buffer `out'.
 *
 * Return the number of bytes of compressed buffer (return 0 on error).
 */
size_t
codec_compress_ints(struct codec *codec,
                   const uint32_t *in, size_t len,
                   void *out)
{
	if (codec->method == CODEC_FOR) {
		struct for_delta_args *args = (struct for_delta_args*)codec->args;
		return for_compress((uint32_t*)in, len, out, &args->b);

	} else if (codec->method == CODEC_FOR_DELTA) {
		struct for_delta_args *args = (struct for_delta_args*)codec->args;
		return for_delta_compress(in, len, (uint32_t*)out, &args->b);

	} else if (codec->method == CODEC_PLAIN) {
		return dummpy_copy(in, len, out);

	} else {
		assert(0);
	}

	return 0;
}

/*
 * Decompress the compressed uint32_t integer array `in' whose original
 * length is `len', using the algorithm and its parameters specified from
 * `codec'. It writes the decompressed content to `out'.
 *
 * Return the number of compressed bytes processed (return 0 on error).
 */
size_t
codec_decompress_ints(struct codec *codec, const void *in,
                      uint32_t *out, size_t len)
{
	if (codec->method == CODEC_FOR) {
		struct for_delta_args *args = (struct for_delta_args*)codec->args;
		return for_decompress((uint32_t*)in, out, len, &args->b);

	} else if (codec->method == CODEC_FOR_DELTA) {
		struct for_delta_args *args = (struct for_delta_args*)codec->args;
		return for_delta_decompress(in, out, len, &args->b);

	} else if (codec->method == CODEC_PLAIN) {
		return dummpy_copy(in, len, out);

	} else {
		assert(0);
	}

	return 0;
}

size_t
codec_compress(struct codec* codec, const void* src, size_t src_sz,
               void** dest)
{
	long unsigned int dest_sz;
	int res;

	switch (codec->method) {
	case CODEC_GZ:
		dest_sz = compressBound(src_sz);
		*dest = malloc(dest_sz);

		res = compress(*dest, &dest_sz /* second parameter is both an
		                                  input and an output */,
		               src, src_sz);

		if (res != Z_OK) {
			dest_sz = 0;
			free(*dest);
			*dest = NULL;
		}

		break;

	default:
		assert(0);
	}

	return dest_sz;
}

size_t
codec_decompress(struct codec* codec, const void* src, size_t src_sz,
                 void* dest, size_t dest_sz /* max destination buffer size */)
{
	int res;
	switch (codec->method) {
	case CODEC_GZ:
		res = uncompress(dest, &dest_sz /* second parameter is both an
	                                       input and an output */,
	                     src, src_sz);

		if (res != Z_OK)
			dest_sz = 0;

		break;

	default:
		assert(0);
	}

	return dest_sz;
}
