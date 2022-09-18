// if file is regular check to see if 8 ytes are png signature
// recursive dfs

/**
 * @file: ls_fname.c
 * @brief: simple ls command to list file names of a directory 
  */

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>   /* for printf().  man 3 printf */
#include <stdlib.h>  /* for exit().    man 3 exit   */
#include <string.h>  /* for strcat().  man strcat   */
#include <sys/queue.h>
#include "helper.h"
void recurse_dir(char* dir_path);
// acount for more than one directory input

int main(int argc, char *argv[]) 
{
    DIR *p_dir;
    struct dirent *p_dirent;
    char str[64];

    if (argc == 1) {
        fprintf(stderr, "Usage: %s <directory name>\n", argv[0]);
        exit(1);
    }

    if ((p_dir = opendir(argv[1])) == NULL) {
        sprintf(str, "opendir(%s)", argv[1]);
        perror(str);
        exit(2);
    }

    while ((p_dirent = readdir(p_dir)) != NULL) {
        char *str_path = p_dirent->d_name;  /* relative path name! */

        if (str_path == NULL) {
            fprintf(stderr,"Null pointer found!"); 
            exit(3);
        } else if ( ( (strcmp(str_path, "..") == 0 ) || ( strcmp(str_path, ".") ) == 0) ){
            continue;
        }
        else {
           // printf("%s\n", str_path );
            char relative_path[64];
            snprintf(relative_path, sizeof(relative_path), "%s/%s", argv[1], str_path);
            if( filetype(relative_path) == 1 ){
                recurse_dir(relative_path);
                // printf("%s\n", relative_path);
            }
        }
    }

    // get ll files and recurse adding to path
    filetype("helper.h");

    if ( closedir(p_dir) != 0 ) {
        perror("closedir");
        exit(3);
    }
    // IMPLEMNT ---> "findpng: No PNG file found" error msg
    return 0;
}

// every level we go down add a /

void recurse_dir(char* dir_path){

    DIR *p_dir;
    struct dirent *p_dirent;
    char str[64];

    p_dir = opendir(dir_path); // Add in path sytax / not found error
    p_dirent = readdir(p_dir);

    while ((p_dirent = readdir(p_dir)) != NULL) {
        char *str_path = p_dirent->d_name;  /* relative path name! */

        if (str_path == NULL) {
            fprintf(stderr,"Null pointer found!"); 
            exit(3);
        }else if ( ( (strcmp(str_path, "..") == 0 ) || ( strcmp(str_path, ".") ) == 0) ){
            continue;
        } else {
            if(strlen(str_path) == 1){
                return;
            }
            printf("STRPATH: %s\n", str_path );
            char relative_path[64];
            snprintf(relative_path, sizeof(relative_path), "%s/%s", dir_path, str_path);
            printf("%s", relative_path);
            if( filetype(relative_path) == 1 ){
                recurse_dir(relative_path);
                // printf("%s", relative_path);
            }
        }
    }

}