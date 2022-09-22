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
#include "crc.h"



/******************************************************************************
 * DEFINED MACROS 
 *****************************************************************************/
#define BUF_LEN  (256*16)
#define BUF_LEN2 (256*32)

/******************************************************************************
 * GLOBALS 
 *****************************************************************************/
U8 gp_buf_def[BUF_LEN2]; /* output buffer for mem_def() */
 /* output buffer for mem_inf() */

/******************************************************************************
 * FUNCTION PROTOTYPES 
 *****************************************************************************/


/******************************************************************************
 * FUNCTIONS 
 *****************************************************************************/
int get_height(FILE* fp);
int is_png( U8 *buf );
void resize(U8** buffer, long int *size);

int main (int argc, char **argv)
{
    int ret = 0;          /* return value for various routines             */
    U64 len_def = 0;      /* compressed data length                        */
    U64 len_inf = 0;      /* uncompressed data length                      */
    int len_concat = 0; // length of concatenated data uncompressed
    int concat_height = 0;
    U32 crc_val = 0;
    long int buf_size = BUF_LEN2;

    U8* gp_buf_inf = malloc(BUF_LEN2*32*8);
   // create png file and add signature and idat ihdr iend manually +  manually then afterwards memset idat
//    /* TESTING INFLATE DEFALTE */
//        ret = mem_inf(gp_buf_inf, &len_inf, data.p_data, data.length);
//    if (ret == 0) { /* success */
//        printf("original len = %d, len_inf = %lu\n", data.length, len_inf);
//    } else { /* failure */
//        fprintf(stderr,"mem_def failed. ret = %d.\n", ret);
//        return ret;
//    }
//    
//    len_concat = len_concat + len_inf;
//    fwrite(gp_buf_inf, len_inf,1,bp);
//    /* END TESTING INFLATE DEFALTE */
    // create file
//    FILE* bp = fopen("tmp_buffer.png", "wb+");
// testing
//    FILE* fp_og = fopen("fp_original.png", "wb+");
//    FILE* fp_modified = fopen("fp_modified.png", "wb+");
    // LOOP writting uncompressed IDAT to file then recompress and look at length and ntoh length and ad length field to IHDR and make IEND data
    for(int i=1; i < argc; i++){
        
        FILE* fp = fopen(argv[i], "rb");
        struct chunk data;
        //struct IHDR ihdr_dat;
        // array of pointers which point to each chunk->data
        // get idat data of every file and push the data onto a stack / new file
        
        /* Step 1.2: Fill the buffer with some data */
        get_png_data_IDAT( &data, fp);
        concat_height = concat_height + get_height(fp);
        // printf("height: %i\n", concat_height);
        //get_png_data_IHDR( &ihdr_dat, fp);

        ret = mem_inf(gp_buf_inf + len_concat, &len_inf, data.p_data, data.length);
        if (ret == 0) { /* success */
            printf("original len = %d, len_inf = %lu\n", data.length, len_inf);
        } else { /* failure */
            fprintf(stderr,"mem_def failed. ret = %d.\n", ret);
            return ret;
        }
/* testing */
//        ret = mem_def(gp_buf_def, &len_def, gp_buf_inf, len_inf, Z_DEFAULT_COMPRESSION);
//        if (ret == 0) { /* success */
//            printf("len_og = %d, len inf = %ld, len_def = %lu\n", data.length, len_inf, len_def);
//        } else { /* failure */
//            fprintf(stderr,"mem_def failed. ret = %d.\n", ret);
//            return ret;
//        }
//
//        
//        fwrite(data.p_data, data.length, 1, fp_og);
//        fwrite(gp_buf_def, len_def,1,fp_modified); 
//        exit(1);
/* end testing */
        len_concat = len_concat + len_inf;

        //if(len_concat >= 0.5*buf_size){
        //    gp_buf_def = realloc(&gp_buf_def, 2*buf_size);
        //    // resize(&gp_buf_inf, &buf_size);
        //}
        

//        fwrite(gp_buf_inf, len_inf,1,bp);

// testing below 
        //    fwrite(data.p_data, data.length, 1, fp_og); // original data before inflation and compression
//        fwrite(gp_buf_inf, len_inf, 1, fp_og);
//        len_concat = len_inf;
//        U8* tmp_buf1 = malloc( len_concat );
//        fread(tmp_buf1, len_concat, 1, bp); // read inflated data into tmp_buf1
//      mem_def(gp_buf_def, &len_def, gp_buf_inf, len_inf, Z_DEFAULT_COMPRESSION);
//        mem_def(gp_buf_def, &len_def, tmp_buf1, len_concat, Z_DEFAULT_COMPRESSION); // deflate data in tmp buffer
//        fwrite(gp_buf_def, len_def,1,fp_modified); // write recompressed data to fp_modified file
//        printf(" original idat data chunk size %d, new decomrpessed size %ld\n", data.length, len_def);
//        exit(0);
        // printf( "buffer file ptr position %ld \n", ftell(bp) );

// done testing
        fclose(fp);

    }
    
    // can probably jsut write to gp_buf_def + concat_len in line 79 and not use file as buffer
    // create new buffer of size len_concat

   

 //   U8* tmp_buf = malloc( len_concat );
 //   fread(tmp_buf, len_concat, 1, bp); // Read in the entire file to a buffer
 
    // recompress data and store in buffer gp_buf_def
 //   ret = mem_def(gp_buf_def, &len_def, tmp_buf, len_concat, Z_DEFAULT_COMPRESSION);
    ret = mem_def(gp_buf_def, &len_def, gp_buf_inf, len_concat, Z_DEFAULT_COMPRESSION);
    if (ret == 0) { /* success */
        printf("len inf all together = %d, len_def = %lu\n", \
               len_concat, len_def);
    } else { /* failure */
        fprintf(stderr,"mem_def failed. ret = %d.\n", ret);
    }
//    fwrite(gp_buf_def, len_def,1,fp_modified);
    // delete file with tmp data and create blank file for png
//    fclose(bp);
//    exit(0);
// testing 

//   fwrite(gp_buf_def, len_def,1,fp_modified);
//   exit(0);
// done testing

    FILE* bp = fopen("tmp_buffer.png", "wb+");
    //fseek(bp, 0, SEEK_SET);
    // write png sig to file copying from the first png
    // make sure at least one arg 
    FILE* fp = fopen(argv[1], "rb");
    
    // copy ihdr from the first file into the new png that will be created so it has all the same info   
    char bytes[33]; // make dynamic to free
    fread(bytes, 33, 1, fp); // read 33 bytes into buffer bytes
    fwrite(bytes, 33, 1, bp);
    //write height to file
    fseek(bp, 20, SEEK_SET);
    concat_height = ntohl(concat_height);
    fwrite( &concat_height, 4, 1, bp);
    // calculate crc for ihdr and add
    fseek(bp, 12, SEEK_SET); // seek to ihdr data field
    fread(bytes, 17, 1, bp); // read the ihdr data field (13 bytes)
    crc_val = htonl(crc(bytes, 17));
    fseek(bp, 29, SEEK_SET);    // go to crc position
    fwrite( &crc_val, 4, 1, bp); //write crc data

    fclose(fp);
//    exit(1);

    // add IDAT data length and chunk type along with data
    unsigned char idat[4] = { 'I', 'D', 'A', 'T'};
    /* merge idat chunk type and deflated data */
    U8* idat_crc_buf = malloc(len_def + 4);
    memcpy(idat_crc_buf, idat, 4); // copy chars into crc buffer
    memcpy(idat_crc_buf+4, gp_buf_def, len_def); // copy chars into crc buffer
    len_def = htonl(len_def);
    fwrite(&len_def, 4,1,bp); // write length of idat chunk
    len_def = ntohl(len_def);
    fwrite(idat_crc_buf, len_def+4,1,bp);
//    fwrite(idat, 4,1,bp); // write "IDAT" chunk type
//    fwrite(gp_buf_def, len_def,1,bp); // write compressed binary data 
    free(idat_crc_buf);
     // write crc

    crc_val = htonl( crc(idat_crc_buf, len_def+4) ); // CRC USES TYPE AND DATA FIELD
    fwrite(&crc_val, 4,1,bp); // write compressed binary data 

    // ADD IEND DATA AND FIX HEIGHT VALUE IN CORRECT ENDIANESS -> try pnginfo function and opening file 
    // iend = 4 bytes of 0 + "IEND" + crc
    unsigned char iend[8] = {0, 0, 0, 0, 'I', 'E', 'N', 'D'};
    fwrite(iend, 8,1,bp);
    crc_val = htonl(crc(&iend[4], 4));
    fwrite(&crc_val, 4,1,bp); // write proper crc
    // change height
//  fopen("tmp_buffer.png", "wb+");

    fclose(bp);
    /* Clean up */
    //free(tmp_buf);
    return 0;
}

void resize(U8** buffer, long int *size){

    U8* tmp = malloc( (*size)*2 );
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

/* Update a running CRC with the bytes buf[0..len-1]--the CRC
   should be initialized to all 1's, and the transmitted value
   is the 1's complement of the final running CRC (see the
   crc() routine below)). */

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