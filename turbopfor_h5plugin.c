#include <stdlib.h>
#include "turbopfor_h5plugin.h"
//#include "zstd.h"
#include <time.h>

#define TURBOPFOR_FILTER 62016

DLL_EXPORT size_t turbopfor_filter(unsigned int flags, size_t cd_nelmts,
								   const unsigned int cd_values[], size_t nbytes,
								   size_t *buf_size, void **buf)
{
	void *outbuf = NULL; /* Pointer to new output buffer */
	void *inbuf = NULL;	 /* Pointer to input buffer */
	inbuf = *buf;

	size_t ret_value = 0;
	size_t origSize = nbytes; /* Number of bytes for output (compressed) buffer */
	//printf("cd_nelmts = %zu \n", cd_nelmts);
	if (flags & H5Z_FLAG_REVERSE)
	{
		/*
		size_t decompSize = ZSTD_getDecompressedSize(*buf, origSize);
		if (NULL == (outbuf = malloc(decompSize)))
			goto error;
		decompSize = ZSTD_decompress(outbuf, decompSize, inbuf, origSize);

		if (*buf != NULL)
			free(*buf);
		*buf = outbuf;
		outbuf = NULL;
		ret_value = (size_t)decompSize;
		*/
	}
	else
	{
		/*
		int aggression;
		if (cd_nelmts > 0)
			aggression = (int)cd_values[0];
		else
			aggression = ZSTD_CLEVEL_DEFAULT;
		if (aggression < 1 )
			aggression = 1 ;
		else if (aggression > ZSTD_maxCLevel())
			aggression = ZSTD_maxCLevel();

		size_t compSize = ZSTD_compressBound(origSize);
		if (NULL == (outbuf = malloc(compSize)))
			goto error;
		clock_t t;
		t = clock();

		compSize = ZSTD_compress(outbuf, compSize, inbuf, origSize, aggression);
		t = clock() - t;

		double time_taken = ((double)t) / CLOCKS_PER_SEC; // in seconds

		printf("fun() took %f seconds to execute \n", time_taken);

		printf(" aggression = %d, origSize =%zu, compSize = %zu \n", aggression, origSize, compSize);
		if (*buf != NULL)
			free(*buf);
		*buf = outbuf;
		*buf_size = compSize;
		outbuf = NULL;
		ret_value = compSize;
		*/
	}
	//if (outbuf != NULL)
	//	free(outbuf);
	return ret_value;

error:
	return 0;
}

const H5Z_class_t turbopfor_H5Filter =
	{
		H5Z_CLASS_T_VERS,
		(H5Z_filter_t)(TURBOPFOR_FILTER),
		1, 1,
		"TurboPFor-Integer-Compression: https://github.com/dbinlbl/H5TurboPFor",
		NULL, NULL,
		(H5Z_func_t)(turbopfor_filter)};

DLL_EXPORT H5PL_type_t H5PLget_plugin_type(void)
{
	return H5PL_TYPE_FILTER;
}

DLL_EXPORT const void *H5PLget_plugin_info(void)
{
	return &turbopfor_H5Filter;
}
