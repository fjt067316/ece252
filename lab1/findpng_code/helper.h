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