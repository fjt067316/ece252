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
#include <stdio.h>

/******************************************************************************
 * DEFINED MACROS 
 *****************************************************************************/

#define PNG_SIG_SIZE    8 /* number of bytes of png image signature data */
#define CHUNK_LEN_SIZE  4 /* chunk length field size in bytes */          
#define CHUNK_TYPE_SIZE 4 /* chunk type field size in bytes */
#define CHUNK_CRC_SIZE  4 /* chunk CRC field size in bytes */
#define DATA_IHDR_SIZE 13 /* IHDR chunk data field size */

/******************************************************************************
 * STRUCTURES and TYPEDEFS 
 *****************************************************************************/
typedef unsigned char U8;
typedef unsigned int  U32;

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
int is_png(char* path);
int get_png_height(struct data_IHDR *buf);
int get_png_width(struct data_IHDR *buf);
int get_png_data_IHDR(struct data_IHDR *out, FILE *fp, long offset, int whence);

/* declare your own functions prototypes here */
// Put below in a header file -> returns bool 0 for false 1 for true
int is_png( char* path ){

    FILE* fp = fopen(path, "rb");
    if (fp == NULL) {
        printf("NOT A PNG FILE BHRUV");
        return 0;
    }
    // read first 8 bytes of png file and make sure they are equyal to : 137 80 78 71 13 10 26 10 -> (decimal)
    // char = 1 byte so 8 chars for 8 bytes // 1 bytes = 8 bits
    char png_tag[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    char buf[8];
    fread(buf, sizeof(buf), 1, fp);
    // check xor == 0
    
    for(int i = 0; i<8;i++){
        if( (png_tag[i] ^ buf[i]) != 0){
            printf("NOT A PNG\n");
            return 0;
        }
    }

    printf("IS A PNG\n");
    return 1;

}