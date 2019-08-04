#ifndef __COMMON_H__
#define __COMMON_H__

#ifndef NATIVE
#include <kernel.h>
#include <nanos.h>
#endif
#include<macro.h>
#include <vfs.h>
#include <alloc.h>
#include<am.h>


inline task_t* get_cur(void){
    int cpu_id=_cpu();
extern task_entry tasks[MAX_TASK];
extern int current_id[MAX_CPU];
    task_t* ret=tasks[current_id[cpu_id]].task;
    return ret;
}
#endif
