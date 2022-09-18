#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


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

/*
int filetype(char *argv)
{
    int i;
    int argc = 1;
    char *ptr;
    struct stat buf;

    printf("%s: ", argv);
    if (lstat(argv, &buf) < 0) {
        perror("lstat error");
    }   
    if      (S_ISREG(buf.st_mode))  ptr = "regular";
    else if (S_ISDIR(buf.st_mode))  ptr = "directory";
    else if (S_ISCHR(buf.st_mode))  ptr = "character special";
    else if (S_ISBLK(buf.st_mode))  ptr = "block special";
    else if (S_ISFIFO(buf.st_mode)) ptr = "fifo";

#ifdef S_ISLNK
    else if (S_ISLNK(buf.st_mode))  ptr = "symbolic link";
#endif

#ifdef S_ISSOCK
    else if (S_ISSOCK(buf.st_mode)) ptr = "socket";
#endif

    else                            ptr = "**unknown mode**";
    printf("%s\n", ptr);
    return 0;
}
*/


// ############################# FUNCTION FROM lab_png.h


#define PNG_SIG_SIZE    8 /* number of bytes of png image signature data */
#define CHUNK_LEN_SIZE  4 /* chunk length field size in bytes */          
#define CHUNK_TYPE_SIZE 4 /* chunk type field size in bytes */
#define CHUNK_CRC_SIZE  4 /* chunk CRC field size in bytes */
#define DATA_IHDR_SIZE 13 /* IHDR chunk data field size */
typedef unsigned char U8;
typedef unsigned int  U32;
// Put below in a header file -> returns bool 0 for false 1 for true
int is_png( U8 *buf ){

    // read first 8 bytes of png file and make sure they are equyal to : 137 80 78 71 13 10 26 10 -> (decimal)
    // char = 1 byte so 8 chars for 8 bytes // 1 bytes = 8 bits
    unsigned char png_tag[8] = {137, 80, 78, 71, 13, 10, 26, 10};

    for(int i = 0; i<8;i++){
        //printf("%d ", buf[i]);
        if( (png_tag[i] ^ buf[i]) != 0){
            // printf("byte %d, expected: %d, actual: %d, NOT A PNG\n", i, png_tag[i], buf[i]);
            return 0;
        }
    }

   // printf("IS A PNG\n");
    return 1;

}