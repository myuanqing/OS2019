#ifndef __ALLOC_H__
#define __ALLOC_H__

#ifndef NATIVE
#include <klib.h>
#endif

#ifdef NATIVE
extern pthread_mutex_t state_mut;
extern pthread_mutex_t bitmap_mut;
extern pthread_mutex_t debug_mut;
#else
extern int state_mut;
extern int bitmap_mut;
void state_lock();
void state_unlock();
void bitmap_lock();
void bitmap_unlock();
/*LOCKDECL(state);
LOCKDECL(bitmap);*/
#endif

extern uintptr_t pm_start, pm_end;
void *kalloc(size_t);
void kfree(void *);

typedef struct free_entry* free_entry_info;
struct free_entry{
    free_entry_info prev;
    free_entry_info next;
};
typedef struct free_entry free_entry;

#define BLOCK_SIZE 24
typedef struct block* block_info;
struct block{
    int size;
    block_info prev;
    block_info next;
    int free;
    int padding;
    char* ptr;
    union{
        char data[1];
        free_entry FE;
    }u;    
};
typedef struct block block;

#define LOG_PAGE_SIZE 12
#define PAGE_SIZE (1<< LOG_PAGE_SIZE)
typedef struct pages* pages_info;
struct pages{
    int size;
    free_entry_info free_head;
    block_info block_head;
};
typedef struct pages pages;



 
#define MAX_PAGES_ENTRY 256
typedef struct ParallelState* ParallelState_info;
struct ParallelState{
    unsigned long int id;
    int pages_num;
    pages pages_entry[MAX_PAGES_ENTRY];
};
typedef struct ParallelState ParallelState;
#define MAX_PARALLEL_NUM 8
extern ParallelState parallel_state[MAX_PARALLEL_NUM];
extern int parallel_num;

#define MAX_PAGE_NUM (1<<15)
extern int page_num;
extern int page_bitmap[MAX_PAGE_NUM];

void pmm_more_init();


#endif