/* gcc main.c -lz -pthread -lcurl
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
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include "helper.h"
#include "shm_stack.h"

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
#define SNAME_EMPTY "/empty1111222" // empty count
#define SNAME_FULL "/full111122" // full count
#define SNAME_MUTEX "/uglymotherfuckinduckling1" // mutex sem name
#define max(a, b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })




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
int concat_50(void** buffer); // array of pointers
int recv_buf_reset( RECV_BUF *ptr );

void worker(int idx, int producers, uint64_t* bitmask, char* url, struct int_stack* stack, void** buf50);
void producer(uint64_t* curr_mask, char* url, struct int_stack* stack);
void consumer(uint64_t* curr_mask, struct int_stack* stack, void** buf50);
void push_buf(void** buf50, RECV_BUF* recv_buf);

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

fout(void* buf){


}

int main( int argc, char** argv ) 
{
    int NUM_CHILD = 4; // PRODUCERS + CONSUMERS
    int BUFFER_SIZE = 3;
    int child_return;
    uint64_t mask = 0;
    uint64_t full_mask = ((uint64_t)1<<50)-1; 
    char url[256];
    struct int_stack* stack;
    int shm_size = sizeof_shm_stack(BUFFER_SIZE);
    pid_t pid=0;
    pid_t cpids[NUM_CHILD];
    // printf("shm_size: %d\n", shm_size);
    int stack_id = shmget( IPC_PRIVATE, shm_size, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    int buf50_id = shmget( IPC_PRIVATE, 50*sizeof(void*), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    int bitmask_id = shmget( IPC_PRIVATE, sizeof(long unsigned int), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR); // issue here with IPC_EXCL maybe?

    sem_unlink(SNAME_EMPTY);
    sem_unlink(SNAME_FULL);
    sem_unlink(SNAME_MUTEX);
    sem_t *empty = sem_open(SNAME_EMPTY, O_CREAT, 0644, 3);
    sem_t *full = sem_open(SNAME_FULL, O_CREAT, 0644, 0);
    sem_t *mutex = sem_open(SNAME_MUTEX, O_CREAT, 0644, 1);

    if(empty == SEM_FAILED || full == SEM_FAILED || mutex == SEM_FAILED){
        perror("sem_open(3) error");
        exit(EXIT_FAILURE);
    }


    int* bitmask = shmat( bitmask_id, NULL, 0); // pointer to bitmask int
    stack = shmat( stack_id, NULL, 0);
    void** buf50; // pointer of pointers
    buf50 = shmat( buf50_id, NULL, 0);

    int i =0;
    int buf_id;
    for(i=0; i < 50; i++){ // create 50 shared buffer segments that consumers can dump into
        buf_id = shmget( IPC_PRIVATE, 300000, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
        buf50[i] = shmat( buf_id, NULL, 0);
    }

    init_shm_stack(stack, BUFFER_SIZE);
    

    if (argc == 1) {
        strcpy(url, IMG_URL); 
    } else {
        strcpy(url, argv[1]);
    }

    printf("%s: URL is %s\n", argv[0], url);
    int t=0;
    for ( t = 0; t < NUM_CHILD; t++) { // One parent forks a child over and over
        
        pid = fork();

        if ( pid > 0 ) {        /* parent proc */
            cpids[t] = pid;
            continue;
        } else if ( pid == 0 ) { /* child proc */
         //   printf("hit: %lu\n", pid);
            worker(t, 2, bitmask, url, stack, buf50);
            exit(0);
            //break; // avoids fork bomb, child process does not interate thorugh for loop calling fork 
        } else {
            perror("fork");
            abort();
        }
        
    }
    


    wait( &child_return );
    
    // Trivially parent process out here

    concat_50(buf50); // concats the 50 items in global buffer of pointers

    return 0;
}

int concat_50(void** buffer){
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
    //for(int i=0; i < 50; i++){
//
    //    struct chunk data;
    //    get_png_data_IDAT( &data, buffer[i]);
    //    //printf("%d\n", data.length);
    //    
    //    concat_height = concat_height + get_height(buffer[i]);
    //    // resize array if we are nearly full or predicted to go over capacity given an average inflation sizae of ~24 times
    //    if(len_concat+data.length*23 > inf_buf_size){
    //        gp_buf_inf = (U8 *) realloc(gp_buf_inf, (len_concat+data.length*24)*2 );
    //        inf_buf_size = (len_concat+data.length*23)*2;
    //    }
//
    //    ret = mem_inf(gp_buf_inf + len_concat, &len_inf, data.p_data, data.length);
    //    if (ret == 0) { /* success */
//  //          printf("original len = %d, len_inf = %lu\n", data.length, len_inf);
    //    } else { /* failure */
    //        fprintf(stderr,"mem_def failed. ret = %d.\n", ret);
    //        return ret;
    //    }
//
    //    len_concat = len_concat + len_inf;
//  //      fclose(fp);
//
    //}
    int data_len;
    char *p_data;
    for(int i=0; i < 50; i++){

        // get concatenated height
        // get concatenated len

         concat_height = concat_height + get_height(buffer[i]);
         memcpy(&data_len, buffer[i]+33, sizeof(int));
         //printf("data_len: %d\n", data_len);
         p_data = buffer[i] + 33 + sizeof(int);

        // resize array if we are nearly full or predicted to go over capacity given an average inflation sizae of ~24 times
        if(len_concat+data_len > inf_buf_size){
            gp_buf_inf = (U8 *) realloc(gp_buf_inf, (len_concat+data_len)*2 );
            inf_buf_size = (len_concat+data_len)*2;
        }

        memcpy(gp_buf_inf+len_concat, p_data, data_len);

        //printf("len: %d\n", data_len);

        len_concat += data_len;


    }

    ret = mem_def(gp_buf_def, &len_def, gp_buf_inf, len_concat, Z_DEFAULT_COMPRESSION);
    if (ret == 0) { /* success */
        printf("len inf all together = %ld, len_def = %lu\n", \
               len_concat, len_def);
    } else { /* failure */
        fprintf(stderr,"mem_def failed. ret = %d.\n", ret);
    }


    FILE* bp = fopen("all.png", "wb+");

    // write png sig to file copying from the first png
    // make sure at least one arg 
//    FILE* fp = fopen("./outputs/0.png", "rb");                  // DO NOT DELETE

    // copy ihdr from the first file into the new png that will be created so it has all the same info   
//    char bytes[33]; // make dynamic to free]
//    fread(bytes, 33, 1, fp); // read 33 bytes into buffer bytes
    fwrite(buffer[0], 33, 1, bp);
    
    //write height to file
    fseek(bp, 20, SEEK_SET);
    concat_height = ntohl(concat_height);
    fwrite( &concat_height, 4, 1, bp);
    // calculate crc for ihdr and add
    fseek(bp, 12, SEEK_SET); // seek to ihdr data field
    fread(buffer[0], 17, 1, bp); // read the ihdr data field (13 bytes)
 
    crc_val = htonl(crc(buffer[0], 17));
    fseek(bp, 29, SEEK_SET);    // go to crc position
    fwrite( &crc_val, 4, 1, bp); //write crc data

    // add IDAT data length and chunk type along with data
    unsigned char idat[4] = { 'I', 'D', 'A', 'T'};
    /* merge idat chunk type and deflated data */
    U8* idat_crc_buf = malloc(len_def + 4);
    
    memcpy(idat_crc_buf, idat, 4); // copy chars into crc buffer
    printf("hit\n");
    memcpy(idat_crc_buf+4, gp_buf_def, len_def); // copy chars into crc buffer
    printf("hit2\n");
    
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

void worker( int idx, int producers, uint64_t* bitmask, char* url, struct int_stack* stack, void** buf50){ // producer and consumer count

    if (idx > producers-1){
        // consumer
//        shmat // sttach mem segment
//        shmdt // detach
//        shmctl // delete mem
        consumer(bitmask, stack, buf50); // sems[0] = remaining
        return;
    } else {
        // producer
        producer(bitmask, url, stack); // worker and producers have their own sems
        printf("WE DID IT \n AAAAAAAAAAAAAAAAAAAAAAA \n");
        return;
    }
}


void producer(uint64_t* curr_mask, char* url, struct int_stack* stack){
    CURL *curl_handle;
    CURLcode res;
    RECV_BUF recv_buf;
    uint64_t full_mask = ((uint64_t)1<<50)-1; 
    char fname[256];
    pid_t pid = getpid();

    //sem_t spots_left = sem_open(SNAME_EMPTY, 0);
    //sem_t* in_q = sem_open(SNAME_FULL, 0);
    //sem_t* mutex = sem_open(SNAME_MUTEX, 0);
    sem_t* spots_left = sem_open(SNAME_EMPTY, 0);
    sem_t* in_q = sem_open(SNAME_FULL, 0);
    sem_t* mutex = sem_open(SNAME_MUTEX, 0);

    recv_buf_init(&recv_buf, 10000);

    curl_global_init(CURL_GLOBAL_DEFAULT);

    /* init a curl session */
    curl_handle = curl_easy_init();

    if (curl_handle == NULL) {
        fprintf(stderr, "curl_easy_init: returned NULL\n");
    }
    
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_cb_curl3); 
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&recv_buf);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_cb_curl); 
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)&recv_buf);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    res = curl_easy_perform(curl_handle);
        
    while(*curr_mask != full_mask){  

        recv_buf_reset( &recv_buf ); // clear buffer every time
        res = curl_easy_perform(curl_handle);

        if( res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
	        // printf("%lu bytes received in memory %p, seq=%d.\n", recv_buf.size, recv_buf.buf, recv_buf.seq);
            if( 0 == ( (uint64_t) *curr_mask & ((uint64_t)1<< recv_buf.seq)) ){  
                *(curr_mask) |= (uint64_t)1 << recv_buf.seq;

                //int s0;
                //int s1;
                //sem_getvalue(spots_left, &s0);
                //sem_getvalue(in_q, &s1);
                
                // create shared mem and store recv buf data there
                //free(recv_buf.buf);
                //recv_buf.buf = new_buf;

                sem_wait(spots_left); // wait until there are spots left in buffer
                sem_wait(mutex); // lock threads to buffer 
                //if( push(stack, recv_buf) != 0){
                //    printf("ERROR ERROR 1 \n");
                //}
                
                push(stack, recv_buf);
                
                

                // int len;
                // memcpy(&len, recv_buf.buf+33, sizeof(U32));
                // len = htonl(len);
                // printf("aa%.*sbb\n", 1000, recv_buf.buf);
                // printf("UH %d\n", len);

                //int len;
                //memcpy(&len, stack->items[stack->pos].buf+33, sizeof(U32));
                //len = htonl(len);
                //printf("aa%.*sbb\n", 1000, stack->items[stack->pos].buf);
                //printf("UH %d\n", len);

                //printf("hit\n");
                //++(stack->pos);
                //stack->items[stack->pos] = *copy_buf(recv_buf);

                //printf("aa%.*sbb\n", stack->items[stack->pos].size, stack->items[stack->pos].buf);
                //printf("aa%.*sbb\n", stack->items[stack->pos].size, copy_buf(recv_buf));
                //printf("aa%.*sbb\n", stack->items[stack->pos].size, stack->items[stack->pos].buf);
                //printf("aa%.*sbb\n", recv_buf.size, recv_buf.buf);
                sem_post(mutex); // back to normal operations
                sem_post(in_q); // Add one to q counter
            //    printf("curr_mask2: %lu\n", *curr_mask);
                
                

            }
        }
    }
        /* cleaning up */
    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();
    recv_buf_cleanup(&recv_buf);
        
  //  return ((void *)p_out);
    }

    // if bad get another image
    // if good check stack full
    // while full wait
    // push stack
    // loop

void consumer(uint64_t* curr_mask, struct int_stack* stack, void** buf50){
    uint64_t full_mask = ((uint64_t)1<<50)-1; 
    RECV_BUF recv_buf;

    sem_t* spots_left = sem_open(SNAME_EMPTY, 0);
    sem_t* in_q = sem_open(SNAME_FULL, 0);
    sem_t* mutex = sem_open(SNAME_MUTEX, 0);

    void* tmp_buf = malloc(10000);

    //int s1 = 0;
    //while(1){
    //    sem_getvalue(&in_q, &s1);
    //    printf("consumer in_q: %d\n", s1);
    //}
    while( (*curr_mask != full_mask) || !is_empty(stack)){  
        //int s0;
        //int s1;
        //sem_getvalue(spots_left, &s0);
        //sem_getvalue(in_q, &s1);
        //printf("consumer spots_left: %d in_q: %d\n", s0, s1);
        
//        recv_buf_reset( &recv_buf );
        sem_wait(in_q); // wait until something in q
        sem_wait(mutex); 
               
        //f( pop(stack, recv_buf) != 0){
        //   printf("ERROR ERROR 2\n");
        //
        //printf("aa%.*sbb\n", stack->items[stack->pos].size, stack->items[stack->pos].buf);
        // recv_buf = stack->items[stack->pos];
        // --(stack->pos);
        //printf("aa%.*sbb\n", stack->items[stack->pos].size, stack->items[stack->pos].buf);
        pop(stack, &recv_buf);
        //printf("aa%.*sbb\n", recv_buf.size, recv_buf.buf);

        memcpy(tmp_buf, recv_buf.buf, recv_buf.size );

        sem_post(mutex);
        sem_post(spots_left); // let em know we yoinked it

        //printf("aa%.*sbb\n", 10, recv_buf.buf);
        //printf("SEQ: %d\n", recv_buf.seq);


        recv_buf.buf = tmp_buf;
        push_buf(buf50, &recv_buf);
        
    }
    
    
}

void push_buf(void** buf50, RECV_BUF *recv_buf){
    
    //create new shared mem location to hold recv_buf
    // copy over to shared mem 
    // put shared mem pointer in buf50
//    printf("BUF50 SEQ ADDED: %d\n", recv_buf.seq);
    //int new_id = shmget( IPC_PRIVATE, recv_buf.size, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    //void* new_mem = shmat( new_id, NULL, 0);
    //buf50[recv_buf.seq] = new_mem;
    //memcpy(new_mem, recv_buf.buf, recv_buf.size);
    //printf("aa%.*sbb\n", recv_buf->size, recv_buf->buf);
    //printf("aa%.*sbb\n", recv_buf->size, recv_buf->buf);

    struct chunk data;
    get_png_data_IDAT( &data, recv_buf->buf);
    //printf("%d\n", data.length);
    
    //int height = get_height(recv_buf->buf);
    int size = 0;
    void* bp = buf50[recv_buf->seq];
    int ret = mem_inf(bp+33+sizeof(int), &size, data.p_data, data.length);

    if (ret == 0) { /* success */
//           printf("original len = %d, len_inf = %lu\n", data.length, len_inf);
    } else { /* failure */
        fprintf(stderr,"mem_def failed. ret = %d.\n", ret);
        return ret;
    }

    memcpy(bp, recv_buf->buf, 33); // copy IHDR to first 33 Bytes
    memcpy(bp+33, &size, sizeof(int)); // write data size to second int index in array

        // len_concat = len_concat + len_inf;

    
    // memcpy(bp, recv_buf->buf, recv_buf->size);

    //int len;
    //memcpy(&len, bp+33, sizeof(U32));
    //len = htonl(len);
    //printf("UH %d\n", len);

    
}
