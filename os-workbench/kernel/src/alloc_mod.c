#include <alloc.h>
#include <kernel.h>
extern uintptr_t pm_start, pm_end;

int state_mut = 0;
int bitmap_mut = 0;

void state_lock(){
  //cli();
  while(_atomic_xchg(&state_mut,1));
}
void state_unlock(){
  _atomic_xchg(&state_mut, 0);
  //sti();
}

void bitmap_lock(){
  //cli();
  while(_atomic_xchg(&bitmap_mut,1));
}
void bitmap_unlock(){
  _atomic_xchg(&bitmap_mut, 0);
  //sti();
}

//LOCKDEF(state);
//LOCKDEF(bitmap);
static void pmm_init() {
  pm_start = (uintptr_t)_heap.start;
  pm_end   = (uintptr_t)_heap.end;
  //printf("start: %x, end: %x", pm_start, pm_end);
  pmm_more_init();
  parallel_num = _ncpu();
  
  assert(page_num >= parallel_num);
  for(int i = 0; i < parallel_num; ++i){
    parallel_state[i].id = i;
    parallel_state[i].pages_num = 1;
    page_bitmap[i] = 0;
    parallel_state[i].pages_entry[0].size = 1;
    parallel_state[i].pages_entry[0].block_head = (block_info)(pm_start+PAGE_SIZE*i);
    parallel_state[i].pages_entry[0].block_head->size = PAGE_SIZE - BLOCK_SIZE;
    parallel_state[i].pages_entry[0].block_head->prev = NULL;
    parallel_state[i].pages_entry[0].block_head->next = NULL;
    parallel_state[i].pages_entry[0].block_head->free = 1;
    parallel_state[i].pages_entry[0].block_head->ptr = parallel_state[i].pages_entry[0].block_head->u.data;
    parallel_state[i].pages_entry[0].block_head->u.FE.prev = NULL;
    parallel_state[i].pages_entry[0].block_head->u.FE.next = NULL;
    parallel_state[i].pages_entry[0].free_head = (free_entry_info)&parallel_state[i].pages_entry[0].block_head->u.FE;
  }


}

MODULE_DEF(pmm) {
  .init = pmm_init,
  .alloc = kalloc,
  .free = kfree,
};