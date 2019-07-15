#include<common.h>
#include<klib.h>
#include<am.h>


static void kmt_spin_init(spinlock_t *lk, const char* name);
static void kmt_spin_lock(spinlock_t *lk);
static void kmt_spin_unlock(spinlock_t *lk);

/*static void idle(){
    while(1);
}*/


task_t kernel_task[MAX_CPU];

struct task_entry{
    task_t* task;
    int used;
};

typedef struct task_entry task_entry;
static task_entry tasks[MAX_TASK];
static spinlock_t entry_lock[MAX_TASK];
int current_id[MAX_CPU];
static int past_id[MAX_CPU];

/*
static void printstack(_Context* context){
    printf("***************************\n");
    printf("%p\n",context->eax);
    printf("%p\n",context->ebx);
    printf("***************************\n");

}*/

static _Context *kmt_context_save(_Event ev, _Context* context){
    
    int index = past_id[_cpu()];
    //printf("[ev]:%d\t[cpu id]:%d\t[past id]:%d\t[current id]:%d\t[state]:%d\n", ev.event,_cpu(),index, current_id[_cpu()], tasks[current_id[_cpu()]].task->state);

    if(index != -1){
        kmt_spin_lock(&entry_lock[index]);
        //tasks[index].task->context = *context;  
    //    printf("saved eip: %p\n", tasks[index].task->context.eip );
        switch(tasks[index].task->state){
            case RUNNING:
            //case SIGNALED:
                tasks[index].task->state = RUNNABLE;
                break;
            case WAITING_TOBE:
                tasks[index].task->state = WAITING;
                break;
            case RUNNABLE_TOBE:
                tasks[index].task->state = RUNNABLE;
                break;
            case KILLED:
                //assert(0);
                tasks[index].used = 0;
                pmm->free(tasks[index].task->stack);
                break;
            default:
                break;
        }
        kmt_spin_unlock(&entry_lock[index]); 
        
    }else{
        //kernel_task[_cpu()].context = *context;        
    }

    int cur_index = current_id[_cpu()];
    if(cur_index != -1){
        tasks[cur_index].task->context = *context;
    }else{
        kernel_task[_cpu()].context = *context;
    }

    past_id[_cpu()] = cur_index;
    current_id[_cpu()] = -1;
    return NULL;
}

static _Context *kmt_context_switch(_Event ev, _Context* context){
    int begin = current_id[_cpu()] +1 ;
//    for(int i = 0;i < _ncpu(); ++i){
//        printf("%d, ",current_id[_cpu()]);
//    }
//    printf("\n");
    for(int i =0;i < MAX_TASK;++i){
        int index = (i + begin )% MAX_TASK;
        assert(index <= MAX_TASK);
        
        //int enable = _intr_read();
        //_intr_write(0);
        kmt_spin_lock(&entry_lock[index]);
        //struct task_entry* task_info = &tasks[index];
        if( tasks[index].used != 0){
            if(tasks[index].task->state == RUNNABLE){
            //printf("cpu: %d ;task: %d\n", _cpu(), index);
                //assert(0);
//                printf("[ev]:%d\t[cpu]:%d\t[id_to_run]:%d\t[state]:%d\n",ev.event,_cpu(),index,tasks[index].task->state);
                tasks[index].task->state = RUNNING;
//                printf("[cpu]:%d\t[STATE]:%d\n",_cpu(),tasks[index].task->state);
                //_intr_write(enable);
                kmt_spin_unlock(&entry_lock[index]);
                assert(tasks[index].task->context.eip < 0x200000);
                current_id[_cpu()] = index;
                
                return &(tasks[index].task->context);
            }
        }
        //_intr_write(enable);
        kmt_spin_unlock(&entry_lock[index]);
    }
    current_id[_cpu()] = -1;
    //printf("return %d, %p\n",-1, kernel_task[_cpu()].context.eip);
    return &(kernel_task[_cpu()].context);
}

static void kmt_init(){
    for(int i = 0;i < MAX_TASK; ++i){
        kmt_spin_init(&entry_lock[i],"entry_lock");
    }
    for(int i = 0;i < MAX_CPU; ++i){
        kernel_task[i].name = "idle";
        //kernel_task[i].stack = pmm->alloc(STACKSIZE);
        kernel_task[i].state = RUNNABLE;
        //_Area  stack = (_Area){kernel_task[i].stack, kernel_task[i].stack + STACKSIZE};
        //kernel_task[i].context =  *_kcontext(stack, idle, NULL);       
    }
    for(int i = 0;i < MAX_CPU; ++i){
        current_id[i] = -1;  
        past_id[i] = -1;      
    }
    for(int i = 0; i < MAX_TASK; ++i){
        tasks[i].used = 0;
    }
    os->on_irq(INT_MIN, _EVENT_NULL, kmt_context_save);
    os->on_irq(INT_MAX, _EVENT_NULL, kmt_context_switch);
}


static int kmt_create(task_t *task, const char * name, void(*entry)(void *arg), void* arg){
    

    if(task == NULL){return -1;}

    task->name = name;
    task->stack = pmm->alloc(STACKSIZE);
    if(task->stack == NULL){
        return -1;
    }
    _Area stack = (_Area){task->stack, task->stack + STACKSIZE};
    task->context = *_kcontext(stack, entry, arg);
    task->state = RUNNABLE;



    int c = -1;
 
    int enable = _intr_read();
    _intr_write(0);

    for(int i = 0;i < MAX_TASK; ++i){
        
        kmt_spin_lock(&entry_lock[i]);
        if(tasks[i].used == 0){
            c = i;
            tasks[i].used = 1;
            tasks[i].task = task;
            
            kmt_spin_unlock(&entry_lock[i]);

            break;
        }
        kmt_spin_unlock(&entry_lock[i]);
    }
    _intr_write(enable);
    if(c == -1){
        pmm->free(task->stack);
        return -1;
    }

    return 0;   
}

static void kmt_teardown(task_t *task){
    

    int enable = _intr_read();
    _intr_write(0);
    for(int i = 0;i < MAX_TASK;++i){
        kmt_spin_lock(&entry_lock[i]);
        if((tasks[i].used == 1)&&(tasks[i].task == task)){
            switch(tasks[i].task->state){
                case RUNNABLE:
                    pmm->free(tasks[i].task->stack);
                    tasks[i].used = 0;
                break;
                case RUNNING:
                    tasks[i].task->state = KILLED;
                break;
                case WAITING:
                    kmt_spin_lock(&tasks[i].task->wait->lock);
                    tasks[i].task->wait->count--;
                    tasks[i].task->wait->wait_queue[i] = 0;
                    kmt_spin_unlock(&tasks[i].task->wait->lock);
                    pmm->free(tasks[i].task->stack);
                    tasks[i].used = 0;
                break;

                default:
                break;
            }            
            kmt_spin_unlock(&entry_lock[i]);
            break;
        }
        kmt_spin_unlock(&entry_lock[i]);
    }


        
    _intr_write(enable);

    return;
}

static void kmt_spin_init(spinlock_t *lk, const char* name){
    lk->name = name;
    lk->locked = 0;
    return;
}

static void kmt_spin_lock(spinlock_t *lk){
    while(_atomic_xchg(&lk->locked,1));
    return;
}

static void kmt_spin_unlock(spinlock_t *lk){
    _atomic_xchg(&lk->locked,0);    
    return;
}

static void kmt_sem_init(sem_t *sem, const char* name, int count){
    sem->name = name;
    sem->count = count;
    for(int i = 0;i < MAX_TASK;++i){
        sem->wait_queue[i] = 0;
    }
    kmt_spin_init(&sem->lock, name);
    return;
}

static void kmt_sem_wait(sem_t *sem){
    int index = current_id[_cpu()];

    int enable = _intr_read();
    _intr_write(0);


    kmt_spin_lock(&sem->lock);
    sem->count--;
    if(sem->count < 0){
//        printf("wait %s, %d\n", sem->name, -sem->count);
        sem->wait_queue[index] = 1;
        kmt_spin_unlock(&sem->lock);

        kmt_spin_lock(&entry_lock[index]);
        if(tasks[index].task->state == RUNNING){
            tasks[index].task->wait = sem;
            tasks[index].task->state = WAITING_TOBE;
        }
        kmt_spin_unlock(&entry_lock[index]);
        _intr_write(enable);
        _yield();
    }else{
        kmt_spin_unlock(&sem->lock);
        _intr_write(enable);
    }
    return;
}

static void kmt_sem_signal(sem_t *sem){

    int enable = _intr_read();
    _intr_write(0);    

    //if( strcmp(sem->name, "keyboard-interrupt")==0){
    //    assert(0);
    //    printf("p\n");
    //}
    if(current_id[_cpu()]!=-1){
        kmt_spin_lock(&entry_lock[current_id[_cpu()]]);
        if(   tasks[current_id[_cpu()]].task->state != RUNNING){
            kmt_spin_unlock(&entry_lock[current_id[_cpu()]]);
            _intr_write(enable);
            return;
        }
        kmt_spin_unlock(&entry_lock[current_id[_cpu()]]);
    }


//    if(strcmp(sem->name, "keyboard-interrupt")==0){
//        assert(0);
//    }
    kmt_spin_lock(&sem->lock);
    if(sem->count < 0){
        for(int i = 0;i < MAX_TASK; ++i){
            kmt_spin_lock(&entry_lock[i]);
            if(sem->wait_queue[i]==1){

                //if(strcmp(sem->name, "keyboard-interrupt")==0){
                //   assert(0);
                //}
                if(tasks[i].task->state == WAITING){            
                    tasks[i].task->state = RUNNABLE;
                }else if(tasks[i].task->state == WAITING_TOBE){
                    tasks[i].task->state = RUNNABLE_TOBE;
                }
                

                sem->wait_queue[i] = 0;
                kmt_spin_unlock(&entry_lock[i]); 
                break;
            }
            kmt_spin_unlock(&entry_lock[i]);    
        }
        
        //printf("signal %s\n", sem->name);
        //assert(index > 0);
    }
    sem->count ++;
    kmt_spin_unlock(&sem->lock);
    _intr_write(enable);
    return;
}
MODULE_DEF(kmt){
    .init = kmt_init,
    .create = kmt_create,
    .teardown = kmt_teardown,    
    .spin_init = kmt_spin_init, 
    .spin_lock = kmt_spin_lock,
    .spin_unlock = kmt_spin_unlock,
    .sem_init = kmt_sem_init,
    .sem_wait = kmt_sem_wait,
    .sem_signal = kmt_sem_signal,
};