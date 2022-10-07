#include "helper.h"
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
int concat(char *png_path);

int concat(U8* gp_buf_inf ,U64* inf_buf_size, U64* len_concat, char *png_path){
    int ret = 0;          /* return value for various routines             */
    U64 len_def = 0;      /* compressed data length                        */
    U64 len_inf = 0;      /* uncompressed data length                      */
//    U64 len_concat = 0; // length of concatenated data uncompressed
    int concat_height = 0;
    U32 crc_val = 0;

    memset(gp_buf_def, 0, BUF_LEN2);

    FILE* fp = fopen(png_path, "rb");  // DO NOT DELETE
        struct chunk data;

        get_png_data_IDAT( &data, fp);
        concat_height = concat_height + get_height(fp);
        // resize array if we are nearly full or predicted to go over capacity given an average inflation sizae of ~24 times
        if(len_concat+data.length*23 > *inf_buf_size){
            gp_buf_inf = (U8 *) realloc(gp_buf_inf, (len_concat+data.length*24)*2 );
            *inf_buf_size = (len_concat+data.length*23)*2;
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