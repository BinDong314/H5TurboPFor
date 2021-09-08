#include <stdlib.h>
#include "turbopfor_h5plugin.h"
//#include "zstd.h"
#include <time.h>

#define TURBOPFOR_FILTER 62016

#include "bitpack.h"
#include "vp4.h"
#include "vint.h"
#include "fp.h"
#include "eliasfano.h"
#include "vsimple.h"
#include "transpose.h"
#include "trle.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef __APPLE__
#include <sys/malloc.h>
#else
#include <malloc.h>
#endif
#ifdef _MSC_VER
#include "vs/getopt.h"
#else
#include <getopt.h>
#endif
#if defined(_WIN32)
#include <windows.h>
#define srand48(x) srand(x)
#define drand48() ((double)(rand()) / RAND_MAX)
#define __off64_t _off64_t
#endif
#include <math.h> // pow,fabs
#include <float.h>

#include "hdf5.h"

#include "conf.h"
#include "time_.h"
#define BITUTIL_IN
#include "bitutil.h"

#if defined(__i386__) || defined(__x86_64__)
#define SSE
#define AVX2
#elif defined(__ARM_NEON) || defined(__powerpc64__)
#define SSE
#endif

#ifndef min
#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(x, y) (((x) > (y)) ? (x) : (y))
#endif

#define CBUF(_n_) (((size_t)(_n_)) * 5 / 3 + 1024 /*1024*/)

typedef enum DataElementType
{
	ELEMENT_TYPE_SHORT = 0,
	ELEMENT_TYPE_USHORT = 1
} DataElementType;

unsigned short zz_map(short i)
{
	return (i << 1) ^ (i >> 15);
}

short zz_unmap(unsigned short n)
{
	return (n >> 1) ^ (-(n & 1));
}

/**
 * @brief the filter for Turbopfor
 * 
 * @param flags : is set by HDF5 for decompress or compress
 * @param cd_nelmts: the # of values in  cd_values
 * @param cd_values: the pointer of the parameter 
 * 			cd_values[0]: type of data:  short (0),  int (1)
 *          cd_values[1]: 0/1 pre-processing method: zipzag  
 *          cd_values[2, -]: size of each dimension of a chunk 
 * @param nbytes : input data size
 * @param buf_size : output data size 
 * @param buf : the pointer to data buffer
 * @return size_t 
 */
DLL_EXPORT size_t turbopfor_filter(unsigned int flags, size_t cd_nelmts,
								   const unsigned int cd_values[], size_t nbytes,
								   size_t *buf_size, void **buf)
{

	size_t ret_value = 0;
	size_t origSize = nbytes; /* Number of bytes for output (compressed) buffer */
	printf("cd_nelmts = %zu, cd_values = ", cd_nelmts);
	for (int i = 0; i < cd_nelmts; i++)
	{
		printf("%d , ", cd_values[i]);
	}
	printf("\n");

	unsigned m = 1;
	unsigned n, l;
	unsigned char *out;
	for (int i = 2; i < cd_nelmts; i++)
	{
		m = m * cd_values[i];
	}

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

		switch (cd_values[0])
		{
		case ELEMENT_TYPE_SHORT:
		{
			n = m * sizeof(unsigned short);
			unsigned char *outbuf_short = (unsigned char *)malloc(CBUF(n) + 1024 * 1024);
			p4ndec16((unsigned char *)*buf, m, (uint16_t *)outbuf_short);
			unsigned short *inbuf_ushort = (unsigned short *)out;

			n = m * sizeof(short);
			out = (unsigned char *)malloc(n);
			unsigned short *ushort_p = (unsigned short *)outbuf_short;
			short *short_p = (short *)out;
			if (cd_values[1])
			{
				for (int i = 0; i < m; i++)
				{
					short_p[i] = zz_unmap(ushort_p[i]);
				}
			}
			break;
			free(outbuf_short);
		}
		default:
			printf("Not supported data type yet !\n");
			goto error;
		}
		if (*buf != NULL)
			free(*buf);
		*buf = out;
		ret_value = n;
	}
	else
	{
		switch (cd_values[0])
		{
		case ELEMENT_TYPE_SHORT:
		{
			n = m * sizeof(unsigned short);
			short *inbuf_short = *buf;
			unsigned short *inbuf_ushort = malloc(CBUF(n) + 1024 * 1024);
			if (cd_values[1])
			{
				for (int i = 0; i < m; i++)
				{
					inbuf_ushort[i] = zz_map(inbuf_short[i]);
				}
			}
			out = (unsigned char *)malloc(CBUF(n) + 1024 * 1024);
			l = p4nenc16(inbuf_ushort, m, out); //l is the byte (n is byte too)
			free(inbuf_ushort);
			inbuf_ushort = NULL;
			break;
		}
		default:
			printf("Not supported data type yet !\n");
			goto error;
		}
		if (*buf != NULL)
			free(*buf);
		*buf = out;
		*buf_size = l;
		ret_value = l;
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
