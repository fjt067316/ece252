// IMPLEMENT ERROR CHECK FUNCTIOPN TO MAKE SURE PNG ISNT CORRUPT??
// make sure all png files have same width #########################################################################
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>   /* for printf().  man 3 printf */
#include <stdlib.h>  /* for exit().    man 3 exit   */
#include <string.h>  /* for strcat().  man strcat   */
#include <sys/queue.h>
#include "helper.h"
#include "zutil.h"
int mem_inf(U8 *dest, U64 *dest_len, U8 *source,  U32 source_len);
int write_file_binary(U8* p_data, U64 data_length, FILE* fp);
// acount for more than one directory input
U8 gp_buf_inf[(256*32)];

int main(int argc, char *argv[]) {  
    // LEN OF INFLATE IS BIGGER THAN IN MEM SO WE NEED A LONGER NUMBER U64
    U64 len_inf = 0;
    // empty out or create tmp file to store concatonated bytes
    FILE* bp = fopen("tmp_buffer.johnmcafee", "wb");
    for(int i=1; i<argc; i++){
        printf("%d %s\n", i, argv[i]);
        FILE* fp = fopen(argv[i], "rb");
        int ret = 0;
        struct chunk data;
        // array of pointers which point to each chunk->data
        // get idat data of every file and push the data onto a stack / new file
        
        get_png_data_IDAT( &data, fp);

        //printf(" data->length : %d\n", data.p_data);
        ret = mem_inf( gp_buf_inf, &len_inf , data.p_data,  data.length);
    if (ret == 0) { /* success */
        printf("data->length : = %d\n", data.length);
    } else { /* failure */
        fprintf(stderr,"mem_def failed. ret = %d.\n", ret);
    }
        //write_file_binary( (gp_buf_inf), len_inf, bp);
        fclose(fp);

    }
    
    fclose(bp);
    return 0;
}
// get idat data field  and uncompress then aoppend together and recompress

int write_file_binary(U8* p_data, U64 data_length, FILE* fp){
    
    fwrite(p_data, data_length,1,fp);
    return 0;
}

int mem_inf(U8 *dest, U64 *dest_len, U8 *source,  U32 source_len)
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
