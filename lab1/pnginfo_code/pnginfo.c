/**
 * @brief: How to pass command line input arguments to main program
 * To execute: <executable> [arg1] [arg2]...[argN] 
 * EXAMPLE: ./cmd_arg.out Hello ECE252 101 201 
 */

#include <stdio.h>	/* printf needs to include this header file */
#include <stdlib.h>
#include <libgen.h>
#include "lab_png.h"

int main(int argc, char *argv[]) 
{
    
    char *filepath = argv[1];
    char* filename = basename( filepath);
    // printf("%s\n", filepath);
    char buf[8];
    FILE* fp = fopen(filepath, "rb");
    fread(buf, sizeof(buf), 1, fp);

    is_png(buf);
    //if ( is_png( filepath ) ) {
    //    get_IHDR( filepath );
    //}

    // try allocating data on heap so its a poiner and do "get_png_data_IHDR( data, fp);" and free(data)
    struct data_IHDR data;
    get_png_data_IHDR( &data, fp);
    printf("%s: %u x %u\n", filename, data.width, data.height );
    return 0;
}

// void get_data_IHDR(*width buffer, * height buffer)


