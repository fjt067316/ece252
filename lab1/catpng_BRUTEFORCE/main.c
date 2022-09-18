/**
 * @biref To demonstrate how to use zutil.c and crc.c functions
 *
 * Copyright 2018-2020 Yiqing Huang
 *
 * This software may be freely redistributed under the terms of MIT License
 */

#include <stdio.h>    /* for printf(), perror()...   */
#include <stdlib.h>   /* for malloc()                */
#include <errno.h>    /* for errno                   */
#include "helper.h"
#include "zutil.h"    /* for mem_def() and mem_inf() */



/******************************************************************************
 * DEFINED MACROS 
 *****************************************************************************/
#define BUF_LEN  (256*16)
#define BUF_LEN2 (256*32)

/******************************************************************************
 * GLOBALS 
 *****************************************************************************/
U8 gp_buf_def[BUF_LEN2]; /* output buffer for mem_def() */
U8 gp_buf_inf[BUF_LEN2]; /* output buffer for mem_inf() */

/******************************************************************************
 * FUNCTION PROTOTYPES 
 *****************************************************************************/

void init_data(U8 *buf, int len);

/******************************************************************************
 * FUNCTIONS 
 *****************************************************************************/

/**
 * @brief initialize memory with 256 chars 0 - 255 cyclically 
 */
void init_data(U8 *buf, int len)
{
    int i;
    for ( i = 0; i < len; i++) {
        buf[i] = i%256;
    }
}

int main (int argc, char **argv)
{
    U8 *p_buffer = NULL;  /* a buffer that contains some data to play with */
    int ret = 0;          /* return value for various routines             */
    U64 len_def = 0;      /* compressed data length                        */
    U64 len_inf = 0;      /* uncompressed data length                      */
    
    /* Step 1: Initialize some data in a buffer */
    /* Step 1.1: Allocate a dynamic buffer */
    p_buffer = malloc(BUF_LEN);
    if (p_buffer == NULL) {
        perror("malloc");
	return errno;
    }

    FILE* fp = fopen(argv[1], "rb");
    struct chunk data;
    //struct IHDR ihdr_dat;
    // array of pointers which point to each chunk->data
    // get idat data of every file and push the data onto a stack / new file
        
    /* Step 1.2: Fill the buffer with some data */
    get_png_data_IDAT( &data, fp);
    //get_png_data_IHDR( &ihdr_dat, fp);

    /* Step 2: Demo how to use zlib utility */
    ret = mem_def(gp_buf_def, &len_def, data.p_data, data.length, Z_DEFAULT_COMPRESSION);
    if (ret == 0) { /* success */
        printf("original len = %d, len_def = %lu\n", data.length, len_def);
    } else { /* failure */
        fprintf(stderr,"mem_def failed. ret = %d.\n", ret);
        return ret;
    }

    /*
    ret = mem_inf(gp_buf_inf, &len_inf, gp_buf_def, len_def);
    if (ret == 0) { // sucess
        printf("original len = %d, len_def = %lu, len_inf = %lu\n", \
               BUF_LEN, len_def, len_inf);
    } else {  // failure
        fprintf(stderr,"mem_def failed. ret = %d.\n", ret);
    }
    */

   // create png file and add signature and idat ihdr iend manually +  manually then afterwards memset idat

    // create file
    FILE* bp = fopen("tmp_buffer.png", "wb");
    
    // copy ihdr from the first file into the new png that will be created so it has all the same info 
    char IHDR[33];
    size_t bytes;
    while (0 < (bytes = fread(IHDR, 1, 33, fp))){
        fwrite(IHDR, 1, bytes, bp);
    }

    // LOOP writting uncompressed IDAT to file then recompress and look at length and ntoh length and ad length field to IHDR and make IEND data
    fwrite(gp_buf_def, len_def,1,bp);
    fclose(bp);
    /* Clean up */
    free(p_buffer); /* free dynamically allocated memory */

    return 0;
}


// MAKE SURE TO USE ntoh() WHEN WRITTING LENGTH OF DATA TO IDAT


/**
 * @brief: deflate in memory data from source to dest.
 *         The memory areas must not overlap.
 * @param: dest U8* output buffer, caller supplies, should be big enough
 *         to hold the deflated data
 * @param: dest_len, U64* output parameter, points to length of deflated data
 * @param: source U8* source buffer, contains data to be deflated
 * @param: source_len U64 length of source data
 * @param: level int compression levels (https://www.zlib.net/manual.html)
 *    Z_NO_COMPRESSION, Z_BEST_SPEED, Z_BEST_COMPRESSION, Z_DEFAULT_COMPRESSION
 * @return =0  on success 
 *         <>0 on error
 * NOTE: 1. the compressed data length may be longer than the input data length,
 *          especially when the input data size is very small.
 */
int mem_def(U8 *dest, U64 *dest_len, U8 *source,  U64 source_len, int level)
{
    z_stream strm;    /* pass info. to and from zlib routines   */
    U8 out[CHUNK];    /* output buffer for deflate()            */
    int ret = 0;      /* zlib return code                       */
    int have = 0;     /* amount of data returned from deflate() */
    int def_len = 0;  /* accumulated deflated data length       */
    U8 *p_dest = dest;/* first empty slot in dest buffer        */

    
    strm.zalloc = Z_NULL;
    strm.zfree  = Z_NULL;
    strm.opaque = Z_NULL;

    ret = deflateInit(&strm, level);
    if (ret != Z_OK) {
        return ret;
    }

    /* set input data stream */
    strm.avail_in = source_len;
    strm.next_in = source;

    /* call deflate repetitively since the out buffer size is fixed
       and the deflated output data length is not known ahead of time */

    do {
        strm.avail_out = CHUNK;
        strm.next_out = out;
        ret = deflate(&strm, Z_FINISH); /* source contains the whole data */
        assert(ret != Z_STREAM_ERROR);
        have = CHUNK - strm.avail_out; 
        memcpy(p_dest, out, have);
        p_dest += have;  /* advance to the next free byte to write */
        def_len += have; /* increment deflated data length         */
    } while (strm.avail_out == 0);

    assert(strm.avail_in == 0);   /* all input will be used  */
    assert(ret == Z_STREAM_END);  /* stream will be complete */

    /* clean up and return */
    (void) deflateEnd(&strm);
    *dest_len = def_len;
    return Z_OK;
}

/**
 * @brief: inflate in memory data from source to dest 
 * @param: dest U8* output buffer, caller supplies, should be big enough
 *         to hold the deflated data
 * @param: dest_len, U64* output parameter, length of inflated data
 * @param: source U8* source buffer, contains zlib data to be inflated
 * @param: source_len U64 length of source data
 * 
 * @return =0  on success
 *         <>0 error
 */
int mem_inf(U8 *dest, U64 *dest_len, U8 *source,  U64 source_len)
{
    z_stream strm;    /* pass info. to and from zlib routines   */
    U8 out[CHUNK];    /* output buffer for inflate()            */
    int ret = 0;      /* zlib return code                       */
    int have = 0;     /* amount of data returned from inflate() */
    int inf_len = 0;  /* accumulated inflated data length       */
    U8 *p_dest = dest;/* first empty slot in dest buffer        */

    /* allocate inflate state 8 */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;        /* no input data being provided   */
    strm.next_in = Z_NULL;    /* no input data being provided   */
    ret = inflateInit(&strm);
    if (ret != Z_OK) {
        return ret;
    }

    /* set input data stream */
    strm.avail_in = source_len;
    strm.next_in = source;

    /* run inflate() on input until output buffer not full */
    do {
        strm.avail_out = CHUNK;
        strm.next_out = out;

        /* zlib format is self-terminating, no need to flush */
        ret = inflate(&strm, Z_NO_FLUSH);
        assert(ret != Z_STREAM_ERROR);    /* state no t clobbered */
        switch(ret) {
        case Z_NEED_DICT:
            ret = Z_DATA_ERROR;  /* and fall through */
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            (void) inflateEnd(&strm);
			return ret;
        }
        have = CHUNK - strm.avail_out;
        memcpy(p_dest, out, have);
        p_dest += have;  /* advance to the next free byte to write */
        inf_len += have; /* increment inflated data length         */
    } while (strm.avail_out == 0 );

    /* clean up and return */
    (void) inflateEnd(&strm);
    *dest_len = inf_len;
    
    return (ret == Z_STREAM_END) ? Z_OK : Z_DATA_ERROR;
}

/* report a zlib or i/o error */
void zerr(int ret)
{
    fputs("zutil: ", stderr);
    switch (ret) {
    case Z_STREAM_ERROR:
        fputs("invalid compression level\n", stderr);
        break;
    case Z_DATA_ERROR:
        fputs("invalid or incomplete deflate data\n", stderr);
        break;
    case Z_MEM_ERROR:
        fputs("out of memory\n", stderr);
        break;
    case Z_VERSION_ERROR:
        fputs("zlib version mismatch!\n", stderr);
    default:
	fprintf(stderr, "zlib returns err %d!\n", ret);
    }
}
