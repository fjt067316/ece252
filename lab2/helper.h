/**
 * @brief  micros and structures for a simple PNG file 
 *
 * Copyright 2018-2020 Yiqing Huang
 *
 * This software may be freely redistributed under the terms of MIT License
 */
#pragma once

/******************************************************************************
 * INCLUDE HEADER FILES
 *****************************************************************************/
#include <errno.h> 
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>   /* for printf().  man 3 printf */
#include <stdlib.h>  /* for exit().    man 3 exit   */
#include <string.h>  /* for strcat().  man strcat   */
#include <sys/queue.h>
#include <arpa/inet.h>
#include <assert.h>
#include "zlib.h"

/******************************************************************************
 * DEFINED MACROS 
 *****************************************************************************/

#define PNG_SIG_SIZE    8 /* number of bytes of png image signature data */
#define CHUNK_LEN_SIZE  4 /* chunk length field size in bytes */          
#define CHUNK_TYPE_SIZE 4 /* chunk type field size in bytes */
#define CHUNK_CRC_SIZE  4 /* chunk CRC field size in bytes */
#define DATA_IHDR_SIZE 13 /* IHDR chunk data field size */

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif

#define CHUNK 16384  /* =256*64 on the order of 128K or 256K should be used */

/*************************************************************************
 * STRUCTURES and TYPEDEFS 
*****************************************************************************/
typedef unsigned char U8;
typedef unsigned int  U32;
typedef unsigned long int U64;

typedef struct chunk {
    U32 length;  /* length of data in the chunk, host byte order */
    U8  type[4]; /* chunk type */
    U8  *p_data; /* pointer to location where the actual data are */
    U32 crc;     /* CRC field  */
} *chunk_p;

/* note that there are 13 Bytes valid data, compiler will padd 3 bytes to make
   the structure 16 Bytes due to alignment. So do not use the size of this
   structure as the actual data size, use 13 Bytes (i.e DATA_IHDR_SIZE macro).
 */
typedef struct data_IHDR {// IHDR chunk data 
    U32 width;        /* width in pixels, big endian   */
    U32 height;       /* height in pixels, big endian  */
    U8  bit_depth;    /* num of bits per sample or per palette index.
                         valid values are: 1, 2, 4, 8, 16 */
    U8  color_type;   /* =0: Grayscale; =2: Truecolor; =3 Indexed-color
                         =4: Greyscale with alpha; =6: Truecolor with alpha */
    U8  compression;  /* only method 0 is defined for now */
    U8  filter;       /* only method 0 is defined for now */
    U8  interlace;    /* =0: no interlace; =1: Adam7 interlace */
} *data_IHDR_p;

/* A simple PNG file format, three chunks only*/
typedef struct simple_PNG {
    struct chunk *p_IHDR;
    struct chunk *p_IDAT;  /* only handles one IDAT chunk */  
    struct chunk *p_IEND;
} *simple_PNG_p;

/******************************************************************************
 * FUNCTION PROTOTYPES 
 *****************************************************************************/
int is_png(U8 *buf);
int get_png_height(struct data_IHDR *buf);
int get_png_width(struct data_IHDR *buf);
int get_png_data_IHDR(struct data_IHDR *out, FILE *fp);
int get_png_data_IDAT(struct chunk *out, FILE *fp);
int mem_def(U8 *dest, U64 *dest_len, U8 *source,  U64 source_len, int level);
void zerr(int ret);
int filetype(char *filepath);

int filetype(char *filepath)
{

    struct stat buf;

   // printf("%s: ", argv);
    if (lstat(filepath, &buf) < 0) {
        perror("lstat error");
    }   
    if      (S_ISREG(buf.st_mode))  return 0; // ptr = "regular";
    else if (S_ISDIR(buf.st_mode))  return 1; //ptr = "directory";
    else if (S_ISCHR(buf.st_mode))  return 2; //ptr = "character special";
    else if (S_ISBLK(buf.st_mode))  return 3; //ptr = "block special";
    else if (S_ISFIFO(buf.st_mode)) return 4; //ptr = "fifo";

#ifdef S_ISLNK
    else if (S_ISLNK(buf.st_mode)) return 5; //ptr = "symbolic link";
#endif

#ifdef S_ISSOCK
    else if (S_ISSOCK(buf.st_mode)) return 6; //ptr = "socket";
#endif

    else                            return 7; //ptr = "**unknown mode**";
    //printf("%s\n", ptr);
    return -1;
}

int is_png( U8 *buf ){

    // read first 8 bytes of png file and make sure they are equyal to : 137 80 78 71 13 10 26 10 -> (decimal)
    // char = 1 byte so 8 chars for 8 bytes // 1 bytes = 8 bits
    unsigned char png_tag[8] = {137, 80, 78, 71, 13, 10, 26, 10};

    for(int i = 0; i<8;i++){
        //printf("%d ", buf[i]);
        if( (png_tag[i] ^ buf[i]) != 0){
            return 0;
        }
    }
    return 1;

}


// get IHDR data
int get_png_data_IHDR(struct data_IHDR *out, FILE *fp) {
    // fseek to offset then read
    // https://www.tutorialspoint.com/c_standard_library/c_function_fseek.htm
    // First 8 bytes are sig then read chunk
    fseek(fp, 8 , SEEK_SET);

    // IHDR should have 00 00 00 0D (13 byte data) + 49 48 44 52
    unsigned char IHDR_CHUNK_INFO[8] = {0, 0, 0, 13, 'I', 'H', 'D', 'R'};
    unsigned char chunk_dat[8];

    fread(chunk_dat, sizeof(chunk_dat), 1, fp);
    // make sure it is ihdr block 
    for(int i = 0; i < 8; i++){
        //printf("%d == %d \n", chunk_dat[i], IHDR_CHUNK_INFO[i]);
        if( ( chunk_dat[i] ^ IHDR_CHUNK_INFO[i] ) != 0 ){
            printf("%d == %d NOT A PNG IHDR CHUNK\n", chunk_dat[i], IHDR_CHUNK_INFO[i]);
            return -1;
        }
    }

    unsigned int sizes[2];
    unsigned int width;
    unsigned int height;
    fseek(fp, 16 , SEEK_SET);
    fread((char*)&sizes, 4, 2, fp);
    width = htonl(sizes[0]);
    height = htonl(sizes[1]);

    out->width = width;
    out->height = height;

    // array of 5 1 byte items which will be the info
    char image_info[5];
    fread(image_info, 1, 5, fp);
    out->bit_depth = image_info[0];
    out->color_type = image_info[1];
    out->compression = image_info[2];
    out->filter = image_info[3];
    out->interlace = image_info[4];

    // crc 
    unsigned int crc;
    fread((char*)&crc, 4, 1, fp);
    // IMPLEMENT CRC CHECKER FOR PNGINFO FUNCTION AND ADD ERROS #########################################
    return 0;
}

int get_png_data_IDAT(struct chunk *out, FILE *fp){

    fseek(fp, 33 , SEEK_SET);
    // read idat data length and make sure chunk type is idat
    // 4 Bytes idat length
    fread( &(out->length) , sizeof(U32), 1, fp); // read chunk data length
    out->length = htonl(out->length);
    fread( &(out->type) , sizeof(U32), 1, fp);
    out->p_data = malloc( out->length ); // malloc(256*16)
    
    fread( out->p_data , out->length, 1, fp);
    // 4B crc
    fread( &(out->crc) , sizeof(U32), 1, fp);
    
    return 0;
}

void resize(U8** buffer, long int *size){

    U8* tmp = (U8*) malloc( (*size)*2 );
    memcpy(tmp, buffer, *size);
    free(*buffer);
    *buffer=tmp;
    *size *= 2;

}

// MAKE SURE TO USE ntoh() WHEN WRITTING LENGTH OF DATA TO IDAT
int get_height(FILE* fp){

    unsigned int height;
    // fseek just to be sure but don thave too
    fseek(fp, 20 , SEEK_SET);
    fread((char*)&height, 4, 1, fp);
    height = htonl(height);
    return height;
}


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

/* CRC STUFF */


/* Table of CRCs of all 8-bit messages. */
unsigned long crc_table[256];

/* Flag: has the table been computed? Initially false. */
int crc_table_computed = 0;

/* Make the table for a fast CRC. */
void make_crc_table(void)
{
    unsigned long c;
    int n, k;

    for (n = 0; n < 256; n++) {
        c = (unsigned long) n;
        for (k = 0; k < 8; k++) {
            if (c & 1)
                c = 0xedb88320L ^ (c >> 1);
            else
                c = c >> 1;
        }
        crc_table[n] = c;
    }
    crc_table_computed = 1;
}
 

unsigned long update_crc(unsigned long crc, unsigned char *buf, int len)
{
    unsigned long c = crc;
    int n;

    if (!crc_table_computed)
        make_crc_table();
    for (n = 0; n < len; n++) {
        c = crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
    }
    return c;
}

/* Return the CRC of the bytes buf[0..len-1]. */
unsigned long crc(unsigned char *buf, int len)
{
    return update_crc(0xffffffffL, buf, len) ^ 0xffffffffL;
}

