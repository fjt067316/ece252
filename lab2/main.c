/*
 * The code is derived from cURL example and paster.c base code.
 * The cURL example is at URL:
 * https://curl.haxx.se/libcurl/c/getinmemory.html
 * Copyright (C) 1998 - 2018, Daniel Stenberg, <daniel@haxx.se>, et al..
 *
 * The paster.c code is 
 * Copyright 2013 Patrick Lam, <p23lam@uwaterloo.ca>.
 *
 * Modifications to the code are
 * Copyright 2018-2019, Yiqing Huang, <yqhuang@uwaterloo.ca>.
 * 
 * This software may be freely redistributed under the terms of the X11 license.
 */

/** 
 * @file main_wirte_read_cb.c
 * @brief cURL write call back to save received data in a user defined memory first
 *        and then write the data to a file for verification purpose.
 *        cURL header call back extracts data sequence number from header.
 * @see https://curl.haxx.se/libcurl/c/getinmemory.html
 * @see https://curl.haxx.se/libcurl/using/
 * @see https://ec.haxx.se/callback-write.html
 */ 


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <curl/curl.h>
#include <stdint.h>
#include <pthread.h>
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

#define IMG_URL "http://ece252-1.uwaterloo.ca:2520/image?img=1"
#define DUM_URL "https://example.com/"
#define ECE252_HEADER "X-Ece252-Fragment: "
#define BUF_SIZE 1048576  /* 1024*1024 = 1M */
#define BUF_INC  524288   /* 1024*512  = 0.5M */

#define max(a, b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

typedef struct recv_buf2 {
    char *buf;       /* memory to hold a copy of received data */
    size_t size;     /* size of valid data in buf in bytes*/
    size_t max_size; /* max capacity of buf in bytes*/
    int seq;         /* >=0 sequence number extracted from http header */
                     /* <0 indicates an invalid seq number */
} RECV_BUF;

struct thread_args              /* thread input parameters struct */
{
    uint64_t *curr_mask;
    uint64_t full_mask;
    char* url;

};

struct thread_ret               /* thread return values struct   */
{
    int sum;
    int product;
};

size_t header_cb_curl(char *p_recv, size_t size, size_t nmemb, void *userdata);
size_t write_cb_curl3(char *p_recv, size_t size, size_t nmemb, void *p_userdata);
int recv_buf_init(RECV_BUF *ptr, size_t max_size);
int recv_buf_cleanup(RECV_BUF *ptr);
int write_file(const char *path, const void *in, size_t len);

void clean_output_dir(char* folder);
int concat_50();
int recv_buf_reset( RECV_BUF *ptr );
void * get_image(void* args);
/**
 * @brief  cURL header call back function to extract image sequence number from 
 *         http header data. An example header for image part n (assume n = 2) is:
 *         X-Ece252-Fragment: 2
 * @param  char *p_recv: header data delivered by cURL
 * @param  size_t size size of each memb
 * @param  size_t nmemb number of memb
 * @param  void *userdata user defined data structurea
 * @return size of header data received.
 * @details this routine will be invoked multiple times by the libcurl until the full
 * header data are received.  we are only interested in the ECE252_HEADER line 
 * received so that we can extract the image sequence number from it. This
 * explains the if block in the code.
 */
size_t header_cb_curl(char *p_recv, size_t size, size_t nmemb, void *userdata)
{
    int realsize = size * nmemb;
    RECV_BUF *p = userdata;
    
    if (realsize > strlen(ECE252_HEADER) &&
	strncmp(p_recv, ECE252_HEADER, strlen(ECE252_HEADER)) == 0) {

        /* extract img sequence number */
	p->seq = atoi(p_recv + strlen(ECE252_HEADER));

    }
    return realsize;
}


/**
 * @brief write callback function to save a copy of received data in RAM.
 *        The received libcurl data are pointed by p_recv, 
 *        which is provided by libcurl and is not user allocated memory.
 *        The user allocated memory is at p_userdata. One needs to
 *        cast it to the proper struct to make good use of it.
 *        This function maybe invoked more than once by one invokation of
 *        curl_easy_perform().
 */

size_t write_cb_curl3(char *p_recv, size_t size, size_t nmemb, void *p_userdata)
{
    size_t realsize = size * nmemb;
    RECV_BUF *p = (RECV_BUF *)p_userdata;
 
    if (p->size + realsize + 1 > p->max_size) {/* hope this rarely happens */ 
        /* received data is not 0 terminated, add one byte for terminating 0 */
        size_t new_size = p->max_size + max(BUF_INC, realsize + 1);   
        char *q = realloc(p->buf, new_size);
        if (q == NULL) {
            perror("realloc"); /* out of memory */
            return -1;
        }
        p->buf = q;
        p->max_size = new_size;
    }

    memcpy(p->buf + p->size, p_recv, realsize); /*copy data from libcurl*/
    p->size += realsize;
    p->buf[p->size] = 0;

    return realsize;
}


int recv_buf_init(RECV_BUF *ptr, size_t max_size)
{
    void *p = NULL;
    
    if (ptr == NULL) {
        return 1;
    }

    p = malloc(max_size);
    if (p == NULL) {
	return 2;
    }
    
    ptr->buf = p;
    ptr->size = 0;
    ptr->max_size = max_size;
    ptr->seq = -1;              /* valid seq should be non-negative */
    return 0;
}

int recv_buf_reset( RECV_BUF *ptr ){
    memset(ptr->buf, 0, ptr->max_size);
    ptr->size = 0;
    ptr->seq = -1;
    return 0;
}

int recv_buf_cleanup(RECV_BUF *ptr)
{
    if (ptr == NULL) {
	return 1;
    }
    
    free(ptr->buf);
    ptr->size = 0;
    ptr->max_size = 0;
    return 0;
}


/**
 * @brief output data in memory to a file
 * @param path const char *, output file path
 * @param in  void *, input data to be written to the file
 * @param len size_t, length of the input data in bytes
 */

int write_file(const char *path, const void *in, size_t len)
{
    FILE *fp = NULL;

    if (path == NULL) {
        fprintf(stderr, "write_file: file name is null!\n");
        return -1;
    }

    if (in == NULL) {
        fprintf(stderr, "write_file: input data is null!\n");
        return -1;
    }

    fp = fopen(path, "wb");
    if (fp == NULL) {
        perror("fopen");
        return -2;
    }

    if (fwrite(in, 1, len, fp) != len) {
        fprintf(stderr, "write_file: imcomplete write!\n");
        return -3; 
    }
    return fclose(fp);
}


int main( int argc, char** argv ) 
{
    int NUM_THREADS = 10; // hardcoded for now

    uint64_t mask = 0;
    uint64_t full_mask = ((uint64_t)1<<50)-1; 
    pthread_t *p_tids = malloc(sizeof(pthread_t) * NUM_THREADS);
    struct thread_args in_params[NUM_THREADS];
    struct thread_ret *p_results[NUM_THREADS];
    char url[256];

    if (argc == 1) {
        strcpy(url, IMG_URL); 
    } else {
        strcpy(url, argv[1]);
    }

    printf("%s: URL is %s\n", argv[0], url);
    
    clean_output_dir("./outputs/");

    for (int i=0; i<NUM_THREADS; i++) {
        in_params[i].curr_mask = &mask;
        in_params[i].full_mask = full_mask;
        in_params[i].url = url;
        pthread_create(p_tids + i, NULL, get_image, in_params+i); 
    }

    /* get it! */
    for (int i=0; i<NUM_THREADS; i++) {
     //   pthread_join(p_tids[i], (void **)&(p_results[i]));
        pthread_join(p_tids[i], NULL);
//        printf("Thread ID %lu joined.\n", p_tids[i]);
        
    }

    /* Concat images by sequence number */
    // concate 50 png files
    concat_50();

    free(p_tids);
    
    /* the memory was allocated in the do_work thread for return values */
    /* we need to free the memory here */
//    for (int i=0; i<NUM_THREADS; i++) {
//        free(p_results[i]);
//    }

    return 0;
}

void * get_image(void* args){
    CURL *curl_handle;
    CURLcode res;
    struct thread_args *p_in = args;
    RECV_BUF recv_buf;
    char fname[256];
    pid_t pid = getpid();
    
    recv_buf_init(&recv_buf, BUF_SIZE);

    curl_global_init(CURL_GLOBAL_DEFAULT);

    /* init a curl session */
    curl_handle = curl_easy_init();

    if (curl_handle == NULL) {
        fprintf(stderr, "curl_easy_init: returned NULL\n");
        return -1;
    }

    /* specify URL to get */
    curl_easy_setopt(curl_handle, CURLOPT_URL, p_in->url);
    
    /* register write call back function to process received data */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_cb_curl3); 
    /* user defined data structure passed to the call back function */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&recv_buf);

    /* register header call back function to process received header data */
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_cb_curl); 
    /* user defined data structure passed to the call back function */
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)&recv_buf);

    /* some servers requires a user-agent field */
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    

    res = curl_easy_perform(curl_handle);
        
    //printf("curr_mask: %lu, ");
    while(*p_in->curr_mask != p_in->full_mask){  

        recv_buf_reset( &recv_buf ); // clear buffer every time
        res = curl_easy_perform(curl_handle);

        if( res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
	        // printf("%lu bytes received in memory %p, seq=%d.\n", recv_buf.size, recv_buf.buf, recv_buf.seq);
            if( 0 == ( (uint64_t) *p_in->curr_mask & ((uint64_t)1<< recv_buf.seq)) ){  
                *(p_in->curr_mask) |= (uint64_t)1 << recv_buf.seq;
                // write file to ouputs folder
//                printf("seq=%d.\n", recv_buf.seq);
                sprintf(fname, "./outputs/%d.png", recv_buf.seq);
                write_file(fname, recv_buf.buf, recv_buf.size);
//                printf("MASK: %lu\n", *p_in->curr_mask );
            }
        }
    }

        /* cleaning up */
    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();
    recv_buf_cleanup(&recv_buf);
    pthread_exit(0);
        
  //  return ((void *)p_out);
    }

void clean_output_dir(char* folder){
    DIR *p_dir = opendir(folder);
    struct dirent *p_dirent;
    char str[64];
    char abs_path[100] = {0};
    while ((p_dirent = readdir(p_dir)) != NULL) {
        char *str_path = p_dirent->d_name;  /* relative path name! */
        if (str_path == NULL) {
            fprintf(stderr,"Null pointer found!"); 
            exit(3);
        } else if ( ( (strcmp(str_path, "..") == 0 ) || ( strcmp(str_path, ".") ) == 0) ){
            continue;
        }
        
        strcat( abs_path, folder);
        strcat( abs_path, p_dirent->d_name);
        remove(abs_path);
        memset( abs_path, 0, 100 );
    }
}


int concat_50(){
    int ret = 0;          /* return value for various routines             */
    U64 len_def = 0;      /* compressed data length                        */
    U64 len_inf = 0;      /* uncompressed data length                      */
    U64 len_concat = 0; // length of concatenated data uncompressed
    int concat_height = 0;
    U32 crc_val = 0;

    U64 inf_buf_size = BUF_LEN2;
    char path[100] = {0};

    U8* gp_buf_inf = malloc(inf_buf_size);
    memset(gp_buf_inf, 0, inf_buf_size);
    memset(gp_buf_def, 0, BUF_LEN2);
    

    // LOOP writting uncompressed IDAT to file then recompress and look at length and ntoh length and ad length field to IHDR and make IEND data
    for(int i=0; i < 50; i++){

        sprintf(path, "./outputs/%d.png", i);
        FILE* fp = fopen(path, "rb");  // DO NOT DELETE
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
//            printf("original len = %d, len_inf = %lu\n", data.length, len_inf);
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
    FILE* fp = fopen("./outputs/0.png", "rb");                  // DO NOT DELETE
    
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