#ifdef NATIVE
#include <stdio.h>
#include <threads.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <pthread.h>
#endif

#include <common.h>

//initialization
uintptr_t pm_start, pm_end;

ParallelState parallel_state[MAX_PARALLEL_NUM];
int parallel_num;
int page_bitmap[MAX_PAGE_NUM];
int page_num;

void pmm_more_init(){
  parallel_num = 0;
  page_num = (pm_end-pm_start)/PAGE_SIZE;
  //printf("%d\n",page_num);
  assert(page_num <= MAX_PAGE_NUM);
  for(int i = 0;i < page_num; ++i){
    page_bitmap[i] = page_num-i;
  }

}


size_t align8(size_t size){
  size_t ret = 0;
  //assert(size>0);
  ret = ((size + 7)>>3) <<3;
  //printf("%x\n",ret);
  assert(ret >= size);
  return ret;
}

int PagesNum(size_t size){
  int ret = 0;
  ret = ((size + BLOCK_SIZE + PAGE_SIZE-1)>> LOG_PAGE_SIZE);
  assert(ret > 0);
  return ret;
}

block_info split(block_info blk_ptr,size_t size){
  
  if( (int)blk_ptr->size >= (int)(size+8+BLOCK_SIZE) ){ //printf("%d %d\n", blk_ptr->size , size + BLOCK_SIZE+8);
    block_info new_blk = (block_info)((uintptr_t)blk_ptr + size + BLOCK_SIZE);
    //printf("size is %d\n", size);
    //printf("old is %x, split at %x\n", blk_ptr, new_blk);
    int new_size = blk_ptr->size - size - BLOCK_SIZE;
    //printf("%d\n",new_size);
    assert(new_size-8 >= 0);
    new_blk->size = new_size;
    new_blk->prev = blk_ptr;
    
    new_blk->next = blk_ptr->next;
    blk_ptr->next = new_blk;
    blk_ptr->size = size;
    if(new_blk->next != NULL){
      new_blk->next->prev = new_blk;
    }
    new_blk->free = 1;
    new_blk->ptr = new_blk->u.data;
    return new_blk;
  }else{
   return NULL;
  }
}

void* find_block(pages_info page_ptr, size_t size){
  free_entry_info FEI = page_ptr->free_head;
  if(FEI != NULL){
    
    free_entry_info fe_ptr = FEI;
    while(fe_ptr!=NULL){
      if( ((block_info)((int)fe_ptr-BLOCK_SIZE))->size >= size){
//        printf("size=%x\n ptr_next=%x\n", ((block_info)((int)fe_ptr-BLOCK_SIZE))->size,((block_info)((int)fe_ptr-BLOCK_SIZE))->next );
//        if(((block_info)((int)fe_ptr-BLOCK_SIZE))->next!=NULL){
//          printf("free: %x\n", ((block_info)((int)fe_ptr-BLOCK_SIZE))->next->free);
//        }
        //printf("here\n");
      
        block_info ret_blk = (block_info)((int)fe_ptr-BLOCK_SIZE);
        block_info blk_ptr = split( ret_blk ,size);
        //modify block & free_info node 
        if(blk_ptr != NULL){
          //printf("here\n");
          blk_ptr->u.FE.prev = fe_ptr->prev;
          blk_ptr->u.FE.next = fe_ptr->next;
          //printf("%u, %u\n", blk_ptr->u.FE.prev, blk_ptr->u.FE.next);
          if(fe_ptr->prev){
            fe_ptr->prev->next = (free_entry_info)&blk_ptr->u.FE;
          }else{
            page_ptr->free_head = (free_entry_info)&blk_ptr->u.FE;
          }
          if(fe_ptr->next){
            fe_ptr->next->prev = (free_entry_info)&blk_ptr->u.FE;
          }
        }else{
          if(fe_ptr->prev){
            fe_ptr->prev->next = fe_ptr->next;
          }
          if(fe_ptr->next){
            fe_ptr->next->prev = fe_ptr->prev;
          }
          if(fe_ptr->prev == NULL && fe_ptr->next == NULL){
            page_ptr->free_head = NULL;  
          }
        }
//        printf("sizeafter=%x\n", ret_blk->size);
//        if(blk_ptr!=NULL){
//          printf("new=%x\n",blk_ptr->size);
//        }
        ret_blk->free = 0;
        return (void*)ret_blk->u.data;
        
      }else{
        fe_ptr = fe_ptr->next;
      } 
    }
    return NULL;
  }
  return NULL;
}


void free_block(pages_info pages_ptr, void* ptr){
  //printf("!!!%x\n", pages_ptr->free_head);
  block_info blk_ptr = (block_info)((int)ptr-BLOCK_SIZE);
  block_info blk_front = blk_ptr->prev;
  block_info blk_back = blk_ptr->next;
  //printf("%x\n",blk_ptr->size);
  blk_ptr->free = 1;

  char front = 'f';
  char back = 'f';


  if(blk_front==NULL && blk_back == NULL){
    blk_ptr->u.FE.next= NULL;
    blk_ptr->u.FE.prev = NULL;
    pages_ptr->free_head = (free_entry_info)&blk_ptr->u.FE;
    pages_ptr->free_head->prev = NULL;
    pages_ptr->free_head->next = NULL;
    return ;
  }else if(blk_front == NULL){
    //printf("here\n");
    if(blk_back->free == 1){
      back = 't';
    }
  }else if(blk_back == NULL){
    //printf("miss\n");
    if(blk_front->free == 1){
      front = 't';
    }
  }else{
    //printf("gg\n");
    if(blk_back->free == 1){
      back = 't';
    }
    if(blk_front->free == 1){
      front = 't';
    }    
  }

//  printf("%c, %c\n", front, back);
  if(front == 'f' && back == 'f'){
    //printf("%x\n", pages_ptr->free_head);
    blk_ptr->u.FE.next = pages_ptr->free_head;
    if(pages_ptr->free_head != NULL){
      blk_ptr->u.FE.next->prev = (free_entry_info)&blk_ptr->u.FE;
    }
    blk_ptr->u.FE.prev = NULL;
    pages_ptr->free_head = (free_entry_info)&blk_ptr->u.FE;
    //assert(pages_ptr->free_head);
    //printf("%x\n", pages_ptr->free_head);
  }else if( front == 'f'){
    free_entry_info entry_ptr = (free_entry_info)&blk_ptr->next->u.FE;
    //assert(entry_ptr);
    free_entry_info entry_front = entry_ptr->prev;
    free_entry_info entry_back = entry_ptr->next;
    //printf("%x ,%x ,%x\n", entry_ptr, entry_front, entry_back);    
    free_entry_info new_entry = (free_entry_info)&blk_ptr->u.FE;
    assert(new_entry);
    new_entry->prev = entry_front;
    new_entry->next = entry_back;
    if(entry_front == NULL){
      assert(pages_ptr);
      pages_ptr->free_head = new_entry;
      //printf("%x\n",pages_ptr->free_head);
    }else{
      entry_front->next = new_entry;
    }
    if(entry_back != NULL){
      entry_back->prev = new_entry;
    }
  }else if( back == 'f'){
    
  }else{
    assert(blk_ptr->next);
    free_entry_info entry_discard =  (free_entry_info)&blk_ptr->next->u.FE;
    free_entry_info entry_front = entry_discard->prev;
    free_entry_info entry_back = entry_discard->next;
    if(entry_front == NULL){
      pages_ptr->free_head = entry_back;
      //printf("%x\n", pages_ptr->free_head);
    }else{
      entry_front->next = entry_back;      
    }
    if(entry_back != NULL){
      entry_back->prev = entry_front;
    }
  }
//  printf("%c, %c\n", front, back);
  if(back == 't'){
    blk_ptr->size = blk_ptr->size + blk_back->size + BLOCK_SIZE;
//    printf("%x\n", blk_back->size);
    blk_ptr->next = blk_back->next;
    if(blk_back->next != NULL){
      //printf("hen!\n");
      blk_back->next->prev = blk_ptr;
    }
  }
  if(front == 't'){
    blk_front->size = blk_front->size + blk_ptr->size + BLOCK_SIZE;
    blk_front->next = blk_ptr->next;
    if(blk_ptr->next != NULL){
      blk_ptr->next->prev = blk_front;
    }
  }



}


int new_pages(int para_no, size_t size){

  int s = PagesNum(size);
  assert(s > 0);

#ifdef NATIVE  
  pthread_mutex_lock(&bitmap_mut);
#else
  bitmap_lock();
#endif
  //printf("fuc");
  int pages_begin = 0;
  char flag = 'f';
  for(int i = 0 ;i <page_num; ++i){
    if(page_bitmap[i] >= s){
      page_bitmap[i] = 0;
      pages_begin = i;
      flag = 't';
      break;
    }
  }
  if(flag != 'f'){
  //assert(flag == 't');
    for(int j = 1;j < s; ++j){
      page_bitmap[pages_begin+j] = 0;
    }
  }

#ifdef NATIVE
  pthread_mutex_unlock(&bitmap_mut);
#else
  bitmap_unlock();
#endif

  if(flag == 'f') {return -1;}
//  printf("          %d, %d\n", pages_begin, pages_begin + s-1);  
  ParallelState_info state_ptr = (ParallelState_info)&parallel_state[para_no];
  pages_info pages_ptr = state_ptr->pages_entry;
  int p_no = state_ptr->pages_num;
  state_ptr->pages_num++;
  assert(state_ptr->pages_num <= MAX_PAGES_ENTRY);

  pages_ptr[p_no].size = s;
  pages_ptr[p_no].block_head = (block_info)(pm_start + pages_begin * PAGE_SIZE);
  pages_ptr[p_no].block_head->size = s*PAGE_SIZE-BLOCK_SIZE;
  pages_ptr[p_no].block_head->prev = NULL;
  pages_ptr[p_no].block_head->next = NULL;
  pages_ptr[p_no].block_head->free = 1;
  pages_ptr[p_no].block_head->ptr = pages_ptr[p_no].block_head->u.data;
  pages_ptr[p_no].block_head->u.FE.prev = NULL;
  pages_ptr[p_no].block_head->u.FE.next = NULL;
  pages_ptr[p_no].free_head =  (free_entry_info)&pages_ptr[p_no].block_head->u.FE;   

  return p_no;


}

void *self_malloc(int para_no, size_t size){
  //printf("here%x\n",size);
  ParallelState_info state_ptr = (ParallelState_info)&(parallel_state[para_no]);
  pages_info page_ptr = (pages_info)state_ptr->pages_entry;
  void* ret = NULL;
  
  size_t s = align8(size);
  //printf("s:%d, size:%d\n", s, size);
  for(int i = 0;i < state_ptr->pages_num;++i){
    
    ret = find_block((pages_info)&page_ptr[i], s);
    if(ret != NULL){
      //printf("here%d\n",i);
      return ret;
    }
  }
  //printf("hh\n");
  if(state_ptr->pages_num >= MAX_PAGES_ENTRY){return NULL;}
  int p_no = new_pages(para_no, s);
  if(p_no == -1){return NULL;}
  void* rt = find_block( (pages_info)&page_ptr[p_no], s );
  return rt;
}

void set_page_free(int page_begin, int page_end){

#ifdef NATIVE
  pthread_mutex_lock(&bitmap_mut);
#else 
  bitmap_lock();
#endif

  //printf("fuuuuuuuuuuuuu\n");
  //printf("begin: %d, end: %d\n", page_begin, page_end);
  int num = page_end - page_begin + 1;
  //printf("%d, %d\n",page_begin , page_end);
  int count = 0;
  for(int j = page_end+1; (j < page_num) && (page_bitmap[j] >0) ; j++){
    count ++;
  }  
  
  for(int i = page_begin-1; (i >= 0) && (page_bitmap[i]>0) ;i--){
    page_bitmap[i] += num+count;
    
  }

  for(int i = page_begin ; i <= page_end ;++i){
    page_bitmap[i] = count + page_end-i+1;
  }

  //for(int i = 0; i < page_num; ++i){
  //  printf("%d: %d\n", i , page_bitmap[i]);
  //}
#ifdef NATIVE
  pthread_mutex_unlock(&bitmap_mut);
#else 
  bitmap_unlock();
#endif

}


void self_free(int para_no, void* ptr){
  //printf("here\n");
  ParallelState_info state_ptr = (ParallelState_info)&(parallel_state[para_no]);
  pages_info pages_entries = (pages_info)state_ptr->pages_entry;
  for(int i = 0;i < state_ptr->pages_num; ++i){
    if( 
        ( (uintptr_t)pages_entries[i].block_head <= (uintptr_t)ptr ) &&   
        ( (uintptr_t)pages_entries[i].block_head +  pages_entries[i].size * PAGE_SIZE >= ( uintptr_t)ptr ) 
      ){
        //printf("%x, %x, %x\n", (uintptr_t)pages_entries[i].block_head, (uintptr_t)pages_entries[i].block_head +  pages_entries[i].size * PAGE_SIZE,(uintptr_t)ptr );
      //printf("h\n");
      //printf("I am pages %d\n", i);
      //pthread_mutex_lock(&debug_mut);
      //if(i == 0){
      //  printf("!%x, %x, %x\n", ptr, ((block_info)((uintptr_t)ptr-BLOCK_SIZE))->size,pages_entries[0].block_head->size);
      //}
      free_block(&pages_entries[i], ptr);
      //if(i == 0){
      //  printf("*%x, %x, %x\n", ptr, ((block_info)((uintptr_t)ptr-BLOCK_SIZE))->size,pages_entries[0].block_head->size);
      //}
      //pthread_mutex_unlock(&debug_mut);
      //assert(pages_entries[0].block_head->size + BLOCK_SIZE <= PAGE_SIZE);
      //printf("here\n");
      if( (i!=0) && 
          (pages_entries[i].block_head->size + BLOCK_SIZE == pages_entries[i].size * PAGE_SIZE)
        ){
          //printf("here\n");
          int page_begin = ((int)pages_entries[i].block_head - pm_start)/PAGE_SIZE;
          int page_end = page_begin + pages_entries[i].size -1 ;
          set_page_free(page_begin, page_end);
          for(int j = i+1; j < state_ptr->pages_num ;++j){
            pages_entries[j-1].size = pages_entries[j].size;
            pages_entries[j-1].free_head = pages_entries[j].free_head;
            pages_entries[j-1].block_head = pages_entries[j].block_head;
          }
          state_ptr->pages_num --;
      }
      
      return;
    }

  }
  assert(0); //printf("cannot reach\n"); 
}





void *kalloc(size_t size) {
//  pthread_mutex_lock(&state_mut);
  if(size == 0 ){
    return NULL;
  }
  //printf("here\n");
//  pthread_mutex_unlock(&state_mut);
  int para_no = 0;

#ifdef NATIVE  
  int para_num = parallel_num;
  int pid = pthread_self();
  //printf("%lu\n",pthread_self());
  for(int i = 0;i < para_num; ++i){
    if(parallel_state[i].id == pid){
      para_no = i;
      break;
    }
  }
#else  
  para_no = _cpu();
#endif

//  printf("%d\n",para_no);
//  pthread_mutex_lock(&debug_mut);
  void* ret = self_malloc(para_no, size); 
//  pthread_mutex_unlock(&debug_mut);
  return ret;
}

void kfree(void *ptr) {
  if(ptr == NULL){return;}
  block_info blk_ptr = (block_info)(ptr-BLOCK_SIZE);
  if( (uintptr_t)ptr >= pm_end || (uintptr_t)ptr<=pm_start-BLOCK_SIZE){return;}
  if(blk_ptr->ptr != blk_ptr->u.data){return;}

  int para_no=-1;  

#ifdef NATIVE
  int pid = pthread_self();  
  int para_num = parallel_num;
  for(int i = 0;i < para_num; ++i){
    if(parallel_state[i].id == pid){
      para_no = i;
      break;
    }
  }
#else
  para_no = _cpu();
//  printf("%d\n", para_no);
#endif

  assert(para_no >= 0);
  //printf("I am thread %d\n", para_no);
//  pthread_mutex_lock(&debug_mut);
  self_free(para_no, ptr);
//  pthread_mutex_unlock(&debug_mut);
}

