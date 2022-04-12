#include <stdlib.h>
#include "turbopfor_h5plugin.h"
//#include "zstd.h"
#include <time.h>

#define TURBOPFOR_FILTER 62016

//#define DEBUG 1

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

#define SetBit(A, k) (A[(k / 32)] |= (1 << (k % 32)))
#define ClearBit(A, k) (A[(k / 32)] &= ~(1 << (k % 32)))
#define TestBit(A, k) (A[(k / 32)] & (1 << (k % 32)))

/**
 * @brief the filter for Turbopfor
 *
 * @param flags : is set by HDF5 for decompress or compress
 * @param cd_nelmts: the # of values in  cd_values
 * @param cd_values: the pointer of the parameter
 * 			cd_values[0]: type of data:  short (0),  int (1)
 *          cd_values[1]: pre-processing method:
 *                        0: nothing
 *                        1: zipzag
 *                        2: abs
 *                        3: plusabsmin
 *
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
#ifdef DEBUG
	printf("cd_nelmts = %zu, cd_values = ", cd_nelmts);
	for (int i = 0; i < cd_nelmts; i++)
	{
		printf("%d , ", cd_values[i]);
	}
	printf("\n");
#endif

	unsigned m = 1;
	unsigned n, l;
	unsigned char *out;
	for (int i = 2; i < cd_nelmts; i++)
	{
		m = m * cd_values[i];
	}

	// unsigned char *A_bitmap_compressed;
	// unsigned A_bitmap_compressed_size;

	int32_t *A;
	int A_n;
#ifdef DEBUG
	clock_t t;
	t = clock();
#endif
	if (flags & H5Z_FLAG_REVERSE)
	{
		void *old_buf = *buf;
		switch (cd_values[0])
		{
		case ELEMENT_TYPE_SHORT:
		{
			n = m * sizeof(unsigned short);
			unsigned char *outbuf_short = (unsigned char *)malloc(CBUF(n) + 1024 * 1024);

			if (cd_values[1] == 2)
			{
				// unsigned char *buf_new;
				memcpy(&A_n, *buf, sizeof(int32_t));
				A = malloc(sizeof(int32_t) * A_n);
				memcpy(A, *buf + sizeof(int32_t), sizeof(int32_t) * A_n);
				// memcpy(buf + sizeof(int32_t) + sizeof(int32_t) * A_n, l, out);
				*buf = (char *)*buf + sizeof(int32_t) + sizeof(int32_t) * A_n;
				// printf("Debug: decode A_n = %d, A[0, 1, 3]= %d, %d, %d\n", A_n, A[0], A[1], A[2]);
			}

			p4ndec16((unsigned char *)*buf, m, (uint16_t *)outbuf_short);
			// unsigned short *inbuf_ushort = (unsigned short *)out;
			n = m * sizeof(short);
			out = (unsigned char *)malloc(n);
			unsigned short *ushort_p = (unsigned short *)outbuf_short;
			short *short_p = (short *)out;

			// if (cd_values[1])
			// {
			// 	for (int i = 0; i < m; i++)
			// 	{
			// 		short_p[i] = zz_unmap(ushort_p[i]);
			// 	}
			// }

			switch (cd_values[1])
			{
			case 0:
				printf("Warning: test only !\n");
				memcpy(ushort_p, short_p, m * sizeof(unsigned short));
				break;
			case 1: // zigzag
			{
				for (int i = 0; i < m; i++)
				{
					short_p[i] = zz_unmap(ushort_p[i]);
				}
				break;
			}
			case 2: // abs
			{
				for (int i = 0; i < m; i++)
				{
					if (TestBit(A, i)) // Get bit for the positive value: 1
					{
						short_p[i] = ushort_p[i];
					}
					else
					{
						short_p[i] = -ushort_p[i];
					}
				}
				free(A);
				A = NULL;
				break;
			}
			default:
				printf("Not supported cd_values[1] !\n");
				goto error;
			}
			free(outbuf_short);
			outbuf_short = NULL;
			break;
		}
		default:
			printf("Not supported data type yet !\n");
			goto error;
		}

#ifdef DEBUG
		t = clock() - t;
		double time_taken = ((double)t) / CLOCKS_PER_SEC; // in seconds
		printf("H5TurboPfor dec : cost %f seconds  \n", time_taken);
#endif
		if (old_buf != NULL)
			free(old_buf);
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
			switch (cd_values[1])
			{
			case 0:
				printf("Warning: test only !\n");
				memcpy(inbuf_ushort, inbuf_short, m * sizeof(unsigned short));
				break;
			case 1: // zigzag
			{
				for (int i = 0; i < m; i++)
				{
					inbuf_ushort[i] = zz_map(inbuf_short[i]);
				}
				break;
			}
			case 2: // abs
			{
				// int A_n;
				if (m % 32 == 0)
				{
					A_n = m / 32;
				}
				else
				{
					A_n = m / 32 + 1;
				}
				// printf("A_n = %d !\n", A_n);
				// int32_t A[A_n];
				A = malloc(sizeof(int32_t) * A_n);

				for (int i = 0; i < A_n; i++)
					A[i] = 0; // Clear the bit array

				// printf("Warning: test only , to add bit array at the end of output buf!\n");
				// This is for test purpose, we need to deal with bitmap
				for (int i = 0; i < m; i++)
				{
					if (inbuf_short[i] >= 0) // Set bit for the positive value to be 1
					{
						SetBit(A, i);
						inbuf_ushort[i] = inbuf_short[i];
					}
					else
					{
						inbuf_ushort[i] = -inbuf_short[i];
					}
				}
#ifdef DEBUG
				printf("Debug: decode A_n = %d, A[0, 1, 3]= %d, %d, %d\n", A_n, A[0], A[1], A[2]);
#endif
				// Compress A does not help here
				// unsigned char *A_bitmap_compressed = (unsigned char *)malloc(CBUF(A_n * sizeof(int32_t)) + 1024 * 1024);
				// A_bitmap_compressed_size is the byte (n is byte too)
				// A_bitmap_compressed_size = p4nenc32(A, A_n, A_bitmap_compressed);
				// printf("Bitmap: ratio = %f (origSize =%zu, compSize = %zu byte) \n", (float)(A_n * 4) / (float)A_bitmap_compressed_size, A_n * 4, A_bitmap_compressed_size);
				// free(A_bitmap_compressed);
				break;
			}
			default:
				printf("Not supported cd_values[1] !\n");
				goto error;
			}

			out = (unsigned char *)malloc(CBUF(n) + 1024 * 1024);
			l = p4nenc16(inbuf_ushort, m, out); // l is the byte (n is byte too)
			free(inbuf_ushort);
			inbuf_ushort = NULL;

			// We need to attach A at the end
			if (cd_values[1] == 2)
			{
				// out =  A_n  : A[0] A[1] -- A[A_n] :  out  --  out + l
				unsigned char *out_old = out;
				unsigned char *out_new = (unsigned char *)malloc(sizeof(int32_t) + sizeof(int32_t) * A_n + l);
				memcpy(out_new, &A_n, sizeof(int32_t));
				memcpy(out_new + sizeof(int32_t), A, sizeof(int32_t) * A_n);
				memcpy(out_new + sizeof(int32_t) + sizeof(int32_t) * A_n, out, l);
				out = out_new;
				free(out_old);
				l = sizeof(int32_t) + sizeof(int32_t) * A_n + l;
				free(A);
			}
			break;
		}
		default:
			printf("Not supported data type yet !\n");
			goto error;
		}

#ifdef DEBUG
		t = clock() - t;
		double time_taken = ((double)t) / CLOCKS_PER_SEC; // in seconds
		printf("H5TurboPfor: ratio = %f (origSize =%u, compSize = %u byte), cost %f seconds  \n", (float)n / (float)l, n, l, time_taken);
#endif

		if (*buf != NULL)
			free(*buf);
		*buf = out;
		*buf_size = l;
		ret_value = l;
	}
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
