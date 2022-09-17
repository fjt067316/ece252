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
#include <arpa/inet.h>
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
int is_png(U8 *buf);
int get_png_height(struct data_IHDR *buf);
int get_png_width(struct data_IHDR *buf);
int get_png_data_IHDR(struct data_IHDR *out, FILE *fp);

/* declare your own functions prototypes here */


// Put below in a header file -> returns bool 0 for false 1 for true
int is_png( U8 *buf ){

    // read first 8 bytes of png file and make sure they are equyal to : 137 80 78 71 13 10 26 10 -> (decimal)
    // char = 1 byte so 8 chars for 8 bytes // 1 bytes = 8 bits
    unsigned char png_tag[8] = {137, 80, 78, 71, 13, 10, 26, 10};

    for(int i = 0; i<8;i++){
        //printf("%d ", buf[i]);
        if( (png_tag[i] ^ buf[i]) != 0){
            printf("byte %d, expected: %d, actual: %d, NOT A PNG\n", i, png_tag[i], buf[i]);
            return 0;
        }
    }

    printf("IS A PNG\n");
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
    // read 4 bytes to get width
    // unsigned int width[1];
    // fseek(fp, 17 , SEEK_SET);
    // fread( width, sizeof(unsigned int), 1, fp);
    // printf("%u", width[0]);
    unsigned int sizes[2];
    unsigned int width;
    unsigned int height;
    // fseek just to be sure but don thave too
    fseek(fp, 16 , SEEK_SET);
    fread((char*)&sizes, 4, 2, fp);
    width = htonl(sizes[0]);
    height = htonl(sizes[1]);
    //printf("%u ", width );
    //printf("%d", height);

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
    //printf("%u", out->interlace);

    // crc 
    unsigned int crc;
    fread((char*)&crc, 4, 1, fp);
    
    return 0;
}