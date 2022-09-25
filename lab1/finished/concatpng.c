/**
 * @biref To demonstrate how to use zutil.c and crc.c functions
 *
 * Copyright 2018-2020 Yiqing Huang
 *
 * This software may be freely redistributed under the terms of MIT License
 */

   /* for errno                   */
#include "helper.h"   /* for mem_def() and mem_inf() */



/******************************************************************************
 * DEFINED MACROS 
 *****************************************************************************/
#define BUF_LEN  (256*16)
#define BUF_LEN2 (256*32*32)

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
    U64 len_concat = 0; // length of concatenated data uncompressed
    int concat_height = 0;
    U32 crc_val = 0;

    U64 inf_buf_size = BUF_LEN2;

  //  long int buf_size = BUF_LEN2;

    U8* gp_buf_inf = malloc(inf_buf_size);
    memset(gp_buf_inf, 0, inf_buf_size);
    memset(gp_buf_def, 0, BUF_LEN2);


    // LOOP writting uncompressed IDAT to file then recompress and look at length and ntoh length and ad length field to IHDR and make IEND data
    for(int i=1; i < argc; i++){
        
        FILE* fp = fopen(argv[i], "rb");  // DO NOT DELETE
        struct chunk data;

        get_png_data_IDAT( &data, fp);
        concat_height = concat_height + get_height(fp);
        // resize array if we are nearly full or predicted to go over capacity given an average inflation sizae of ~24 times
        if(len_concat+data.length*23 > inf_buf_size){
            gp_buf_inf = (U8 *) realloc(gp_buf_inf, (len_concat+data.length*24)*2 );
            inf_buf_size = (len_concat+data.length*23)*2;
        }

        ret = mem_inf(gp_buf_inf + len_concat, &len_inf, data.p_data, data.length);
        if (ret == 0) { /* success */
            printf("original len = %d, len_inf = %lu\n", data.length, len_inf);
        } else { /* failure */
            fprintf(stderr,"mem_def failed. ret = %d.\n", ret);
            return ret;
        }

        len_concat = len_concat + len_inf;
        fclose(fp);

    }

    ret = mem_def(gp_buf_def, &len_def, gp_buf_inf, len_concat, Z_DEFAULT_COMPRESSION);
    if (ret == 0) { /* success */
        printf("len inf all together = %ld, len_def = %lu\n", \
               len_concat, len_def);
    } else { /* failure */
        fprintf(stderr,"mem_def failed. ret = %d.\n", ret);
    }

    FILE* bp = fopen("concat.png", "wb+");

    // write png sig to file copying from the first png
    // make sure at least one arg 
    FILE* fp = fopen(argv[1], "rb");                  // DO NOT DELETE
    
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

    crc_val = htonl( crc(idat_crc_buf, len_def+4) ); // CRC USES TYPE AND DATA FIELD
    fwrite(&crc_val, 4,1,bp); // write compressed binary data 
    
    // ADD IEND DATA AND FIX HEIGHT VALUE IN CORRECT ENDIANESS -> try pnginfo function and opening file 
    // iend = 4 bytes of 0 + "IEND" + crc
    unsigned char iend[8] = {0, 0, 0, 0, 'I', 'E', 'N', 'D'};
    fwrite(iend, 8,1,bp);
    crc_val = htonl(crc(&iend[4], 4));
    fwrite(&crc_val, 4,1,bp); // write proper crc
    
    /* Clean up */
    free(idat_crc_buf);
    fclose(bp);
    return 0;
}
