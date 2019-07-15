#include <stdio.h>
#include <threads.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <common.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/unistd.h> 
#include <pthread.h>
uintptr_t pm_start, pm_end;
pthread_mutex_t state_mut;
pthread_mutex_t bitmap_mut;
pthread_mutex_t debug_mut;
void worker() {
    int id = pthread_self();

    pthread_mutex_lock(&state_mut);    
    int para_num = parallel_num;
    parallel_num++;
    assert(parallel_num<=MAX_PARALLEL_NUM);
 
    
    parallel_state[para_num].id = pthread_self();
    pthread_mutex_unlock(&state_mut);
    parallel_state[para_num].pages_num = 1;
    int page_no = 0;
    char flag = 'f';
    int rec_of_begin = 0;
    
    pthread_mutex_lock(&bitmap_mut);
    for(int i = 0 ;i <page_num; ++i){
        if(page_bitmap[i] > 0){
            page_bitmap[i] = 0;
            page_no = i;
            flag = 't';
            break;
        }else{
            rec_of_begin = i;
        }
    }
    assert(flag == 't');
    for(int i = rec_of_begin+1;i < page_no; ++i){
        page_bitmap[i]--;   
        assert(page_bitmap[i]>0);
    }
    pthread_mutex_unlock(&bitmap_mut);

    pages_info ptr = (pages_info)&parallel_state[para_num].pages_entry;
    ptr->size = 1;
    ptr->block_head = (block_info)(pm_start + page_no * PAGE_SIZE);
    //printf("%x\n", ptr->block_head);
    ptr->block_head->size = PAGE_SIZE-BLOCK_SIZE;
    ptr->block_head->prev = NULL;
    ptr->block_head->next = NULL;
    ptr->block_head->free = 1;
    ptr->block_head->ptr = ptr->block_head->u.data;
    ptr->block_head->u.FE.prev = NULL;
    ptr->block_head->u.FE.next = NULL;
    ptr->free_head =  (free_entry_info)&ptr->block_head->u.FE;   

    
//    void* a1 = kalloc(0x10);
//    sleep(1);
//    printf("a1: %x\n", a1);
//    void* a2 = kalloc(0x1fff);
//    printf("a2: %x\n",a2);
//    void* a3 = kalloc(0x1999);
//    printf("a3: %x\n",a3);
//    sleep(1);
//    kfree(a1);
//    kfree(a2);
//    kfree(a3);

//    void* b1 = kalloc(0x10);
//    sleep(1);
//    printf("b1: %x\n", a1);
//    void* b2 = kalloc(0x1fff);
//    printf("b2: %x\n",a2);
//    void* b3 = kalloc(0x1999);
//    printf("b3: %x\n",b3);
//    sleep(1);
//   kfree(b1);
//    kfree(b2);
//    kfree(b3); 
    //printf("here\n");    
//    sleep(1);
    void* p[32];
    for (int i = 0; i < 16; i++) {
        int c = rand()&0xff;

        //printf("*%x\n", c);
        p[i] = kalloc(c);
        //block_info bin = parallel_state[0].pages_entry[0].block_head;
        //while(bin!=NULL){
        //    printf("************%p\n", bin);
        //    bin = bin->next;
       //}
 //       if(p[i] != NULL){
//            printf("*%x, %x\n", c, ((block_info)(p[i]-BLOCK_SIZE))->size);
//        }
        //kfree(p);
        usleep(10);
    }
    
    for(int i  = 0;i < 16; ++i){
        kfree(p[i]);
        //block_info bin = parallel_state[0].pages_entry[0].block_head;
        //while(bin!=NULL){
        //    printf("************%p\n", bin);
        //    bin = bin->next;
        //}        
    }
//    printf("ha\n");
    for(int i = 0;i < 32; ++i){
        int c = rand()&0xfff;

        //printf("*%x\n", c);
        p[i] = kalloc(c);
    //    if(p[i] != NULL){
    //        printf("~~~~~%x, %x, %x\n", c, p[i], ((block_info)(p[i]-BLOCK_SIZE))->next );
    //    }
        //kfree(p[i]);
        usleep(10);
    }//printf("here\n");
    for(int i  = 0;i < 32; ++i){
        kfree(p[i]);
        
    }
}

void pmm_init(){
    pm_start = (uintptr_t)malloc(PAGE_SIZE*16);
    assert(pm_start);
    pm_end = (pm_start + PAGE_SIZE*16);
    pmm_more_init();
}

void print(){
    for(int i = 0;i < page_num; ++i){
        printf("%d: %d\n",i , page_bitmap[i]);
    }
    printf("machine :\n");
    for(int i = 0;i < parallel_num;++i){
        printf("%x: %x, %d, %d, %x\n", i , parallel_state[i].pages_entry[0].block_head->size,parallel_state[i].pages_entry[0].block_head->free, 
            parallel_state[i].pages_num, (uintptr_t)parallel_state[i].pages_entry[0].block_head->next);
        
    }
}
int main() {
    pmm_init();
    pthread_mutex_init(&state_mut,NULL);
    pthread_mutex_init(&bitmap_mut,NULL);
    pthread_mutex_init(&debug_mut,NULL);
    for (int i = 0; i < 8; i++)
        create(worker);
    join(print);
    //sleep(10);

    return 0;
}