
/* The code is 
 * Copyright(c) 2018-2019 Yiqing Huang, <yqhuang@uwaterloo.ca>.
 *
 * This software may be freely redistributed under the terms of the X11 License.
 */
/**
 * @brief  stack to push/pop integer(s), API header  
 * @author yqhuang@uwaterloo.ca
 */
#include <stdio.h>
#include <stdlib.h>
struct int_stack;

int sizeof_shm_stack(int size);
int init_shm_stack(struct int_stack *p, int stack_size);
struct int_stack *create_stack(int size);
void destroy_stack(struct int_stack *p);
int is_full(struct int_stack *p);
int is_empty(struct int_stack *p);

typedef struct recv_buf2 {
    char *buf;       /* memory to hold a copy of received data */
    size_t size;     /* size of valid data in buf in bytes*/
    size_t max_size; /* max capacity of buf in bytes*/
    int seq;         /* >=0 sequence number extracted from http header */
                     /* <0 indicates an invalid seq number */
} RECV_BUF;

int push(struct int_stack *p, RECV_BUF item);
int pop(struct int_stack *p, RECV_BUF *p_item);

/*
 * The code is derived from 
 * Copyright(c) 2018-2019 Yiqing Huang, <yqhuang@uwaterloo.ca>.
 *
 * This software may be freely redistributed under the terms of the X11 License.
 */

/**
 * @brief  stack to push/pop integers.   
 */


/* a stack that can hold integers */
/* Note this structure can be used by shared memory,
   since the items field points to the memory right after it.
   Hence the structure and the data items it holds are in one
   continuous chunk of memory.

   The memory layout:
   +===============+
   | size          | 4 bytes
   +---------------+
   | pos           | 4 bytes
   +---------------+
   | items         | 8 bytes
   +---------------+
   | items[0]      | 4 bytes
   +---------------+
   | items[1]      | 4 bytes
   +---------------+
   | ...           | 4 bytes
   +---------------+
   | items[size-1] | 4 bytes
   +===============+
*/
typedef struct int_stack
{
    int size;               /* the max capacity of the stack */
    int pos;                /* position of last item pushed onto the stack */
    RECV_BUF *items;             /* stack of stored integers */
    void *buf;              /* Buffer to hold recv_buf and its data*/
} ISTACK;

/**
 * @brief calculate the total memory that the struct int_stack needs and
 *        the items[size] needs.
 * @param int size maximum number of integers the stack can hold
 * @return return the sum of ISTACK size and the size of the data that
 *         items points to.
 */

int sizeof_shm_stack(int size)
{
    //return (sizeof(ISTACK) + sizeof(int) * size);
    return (sizeof(ISTACK) + 10000 * size);
}

/**
 * @brief initialize the ISTACK member fields.
 * @param ISTACK *p points to the starting addr. of an ISTACK struct
 * @param int stack_size max. number of items the stack can hold
 * @return 0 on success; non-zero on failure
 * NOTE:
 * The caller first calls sizeof_shm_stack() to allocate enough memory;
 * then calls the init_shm_stack to initialize the struct
 */
int init_shm_stack(ISTACK *p, int stack_size)
{
    if ( p == NULL || stack_size == 0 ) {
        return 1;
    }

    p->size = stack_size;
    p->pos  = -1;
    p->items = (RECV_BUF *) (p + sizeof(ISTACK));
    int buf_id = shmget( IPC_PRIVATE, 10000*stack_size, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    p->buf = shmat( buf_id, NULL, 0);
    
    return 0;
}

/**
 * @brief create a stack to hold size number of integers and its associated
 *      ISTACK data structure. Put everything in one continous chunk of memory.
 * @param int size maximum number of integers the stack can hold
 * @return NULL if size is 0 or malloc fails
 */

ISTACK *create_stack(int size)
{
    int mem_size = 0;
    ISTACK *pstack = NULL;
    
    if ( size == 0 ) {
        return NULL;
    }

    mem_size = sizeof_shm_stack(size);
    pstack = malloc(mem_size);

    if ( pstack == NULL ) {
        perror("malloc");
    } else {
        char *p = (char *)pstack;
        pstack->items = (RECV_BUF *) (p + sizeof(ISTACK));
        pstack->size = size;
        pstack->pos  = -1;
    }

    return pstack;
}

/**
 * @brief release the memory
 * @param ISTACK *p the address of the ISTACK data structure
 */

void destroy_stack(ISTACK *p)
{
    if ( p != NULL ) {
        free(p);
    }
}

/**
 * @brief check if the stack is full
 * @param ISTACK *p the address of the ISTACK data structure
 * @return non-zero if the stack is full; zero otherwise
 */

int is_full(ISTACK *p)
{
    if ( p == NULL ) {
        return 0;
    }
    return ( p->pos == (p->size -1) );
}

/**
 * @brief check if the stack is empty 
 * @param ISTACK *p the address of the ISTACK data structure
 * @return non-zero if the stack is empty; zero otherwise
 */

int is_empty(ISTACK *p)
{
    if ( p == NULL ) {
        return 0;
    }
    return ( p->pos == -1 );
}

/**
 * @brief push one integer onto the stack 
 * @param ISTACK *p the address of the ISTACK data structure
 * @param int item the integer to be pushed onto the stack 
 * @return 0 on success; non-zero otherwise
 */

int push(ISTACK *p, RECV_BUF item)
{
    if ( p == NULL ) {
        return -1;
    }

    if ( !is_full(p) ) {

        if(p->pos == -1){
            void* bp = p->buf;
//            printf("hit\n");
            memcpy(bp, item.buf, item.size); // seg fault
            item.buf = bp;
            ++(p->pos);
            p->items[p->pos] = item;
            return 0;
        }
        
        void* bp = p->items[p->pos].buf + p->items[p->pos].size+1;
        memcpy(bp, item.buf, item.size);
        ++(p->pos);
        item.buf = bp;
        p->items[p->pos] = item;
        return 0;
    } else {
        return -1;
    }
}

/**
 * @brief push one integer onto the stack 
 * @param ISTACK *p the address of the ISTACK data structure
 * @param int *item output parameter to save the integer value 
 *        that pops off the stack 
 * @return 0 on success; non-zero otherwise
 */

int pop(ISTACK *p, RECV_BUF *p_item)
{
    if ( p == NULL ) {
        return -1;
    }

    if ( !is_empty(p) ) {

        *p_item = p->items[p->pos];
        (p->pos)--;
        return 0; // return size
    } else {
        return 1;
    }
}
