#include <common.h>
#include <klib.h>
//#include<threads.h>


#define HANDLER_MAX_NUM 0x80
static struct handler_entry{
  handler_t handler;
  int event;
  int seq;
  int next;
}handlers[HANDLER_MAX_NUM]={
  {.next = -1},
};
static int handler_num = 0;


static void os_init() {
  pmm->init();
  kmt->init();
  //_vme_init(pmm->alloc, pmm->free);
  dev->init();
  vfs->init();
  //_putc('1');
  //_putc('\n');
}

static void hello() {
  for (const char *ptr = "Hello from CPU #"; *ptr; ptr++) {
    _putc(*ptr);
    
  }
  
  _putc("12345678"[_cpu()]); _putc('\n');  
//  void* p = pmm->alloc(1);
//  pmm->free(p);
//  pmm->free(p-1);


/*  void*p[16];
  for(int i = 0; i < 16; ++ i){
    int c = rand()&0xfff;
    //printf("%x\n", c);
    p[i] = pmm->alloc(c);
    //pmm->free(p[i]);
    //printf("%p\n",p[i]);
    if(p[i] > 0){assert((uintptr_t)p[i] > pm_start);}
    
  }
//  printf("here\n");
  for(int i = 0; i<16; ++i){
    pmm->free(p[i]);
  }
//  printf("end\n");
  for(int i = 0; i < 16; ++ i){
    int c = rand()&0xfff;
  //  printf("%p\n", p[i]);
    p[i] = pmm->alloc(c);
    //pmm->free(p[i]);
  //  printf("%p\n",p[i]);
  }
  //printf("here\n");
  for(int i = 0; i<16; ++i){
    pmm->free(p[i]);
  }
*/
}

static void os_run() {
  
  hello();   
  
  _intr_write(1);
  while (1) {
    //printf("hello\n");
    //printf("%d\n", _intr_read());
    //_yield();
  }
}


static _Context *os_trap(_Event ev, _Context *context) {
  _Context *ret = NULL;
  int iter = handlers[0].next;
  //int c = 0;
  while(iter != -1){
    //c++;
    //printf("%d\n",c);
    if( (handlers[iter].event == _EVENT_NULL) || (handlers[iter].event == ev.event)){
      _Context* next = handlers[iter].handler(ev, context);
      if(next) ret = next;
    }
    iter = handlers[iter].next;
  }
  return ret;
  
  //return context;
}

static void os_on_irq(int seq, int event, handler_t handler) {
  handler_num++;
  assert(handler_num < HANDLER_MAX_NUM);
  if(handler_num >= HANDLER_MAX_NUM){return;}
  handlers[handler_num].handler = handler;
  handlers[handler_num].event = event;
  handlers[handler_num].seq = seq;
  handlers[handler_num].next = -1;
  int iter = 0;
  int next = handlers[iter].next;
  while(next!=-1){
    if((handlers[next].event == event) && (handlers[next].seq > seq)){
      break;
    }else{
      iter = next;
      next = handlers[iter].next;
    }
  }
  handlers[handler_num].next = next;
  handlers[iter].next = handler_num;
  
}

MODULE_DEF(os) {
  .init   = os_init,
  .run    = os_run,
  .trap   = os_trap,
  .on_irq = os_on_irq,
};
