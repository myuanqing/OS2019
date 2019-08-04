#ifndef __KERNEL_H__
#define __KERNEL_H__

#include <am.h>
#include <nanos.h>

#define MAX_CPU 8
#define MAX_TASK 0x40



#define MODULE(name) \
  mod_##name##_t; \
  extern mod_##name##_t *name
#define MODULE_DEF(name) \
  extern mod_##name##_t __##name##_obj; \
  mod_##name##_t *name = &__##name##_obj; \
  mod_##name##_t __##name##_obj = 

typedef _Context *(*handler_t)(_Event, _Context *);
  typedef struct {
  void (*init)();
  void (*run)();
  _Context *(*trap)(_Event ev, _Context *context);
  void (*on_irq)(int seq, int event, handler_t handler);
} MODULE(os);

typedef struct {
  void (*init)();
  void *(*alloc)(size_t size);
  void (*free)(void *ptr);
} MODULE(pmm);

typedef struct task task_t;
typedef struct spinlock spinlock_t;
typedef struct semaphore sem_t;


typedef struct filesystem filesystem_t;
typedef struct file file_t;
typedef struct fsops fsops_t;
typedef struct inode inode_t;
typedef struct inodeops inodeops_t;

typedef enum TASK_STATE{RUNNABLE=0, RUNNING, WAITING,KILLED, WAITING_TOBE, RUNNABLE_TOBE} TASK_STATE;
#define STACKSIZE 4096
#define NOFILE 16
struct task{
  const char* name;
  file_t *fd[NOFILE];  
  inode_t* cur_dir;
  TASK_STATE state;
  inode_t* cur_inode;
  sem_t* wait;
  _Context context;
  char* stack;
  char pwd[0x100]; 
};


struct task_entry{
    task_t* task;
    int used;
};

typedef struct task_entry task_entry;


struct spinlock{
    const char* name;
    int locked;
};
struct semaphore{
    const char* name;
    spinlock_t lock;
    int count;
    int wait_queue[MAX_TASK];
    

};


typedef struct {
  void (*init)();
  int (*create)(task_t *task, const char *name, void (*entry)(void *arg), void *arg);
  void (*teardown)(task_t *task);
  void (*spin_init)(spinlock_t *lk, const char *name);
  void (*spin_lock)(spinlock_t *lk);
  void (*spin_unlock)(spinlock_t *lk);
  void (*sem_init)(sem_t *sem, const char *name, int value);
  void (*sem_wait)(sem_t *sem);
  void (*sem_signal)(sem_t *sem);
} MODULE(kmt);


typedef struct device  device_t;
typedef struct devops {
  int (*init)(device_t *dev);
  ssize_t (*read)(device_t *dev, off_t offset, void *buf, size_t count);
  ssize_t (*write)(device_t *dev, off_t offset, const void *buf, size_t count);
} devops_t;



typedef struct {
  void (*init)();
} MODULE(dev);
device_t *dev_lookup(const char *name);


extern int current_id[MAX_CPU];

/*****************************************************file system**************************************************/

extern inode_t* root_inode;

struct filesystem {
  const char* name;
  inode_t *inodes, *root, *root_parent;
  fsops_t *ops;
  device_t *dev;
};


struct fsops {
  void (*init)(struct filesystem *fs, const char *name, device_t *dev);
  inode_t *(*lookup)(filesystem_t *fs, const char *path, int flags);
  int (*close)(inode_t *inode);
};




struct inodeops {
  int (*open)(file_t *file, int flags);
  int (*close)(file_t *file);
  ssize_t (*read)(file_t *file, char *buf, size_t size);
  ssize_t (*write)(file_t *file, const char *buf, size_t size);
  off_t (*lseek)(file_t *file, off_t offset, int whence);
  int (*mkdir)(inode_t* inode, const char *name);
  int (*rmdir)(inode_t* inode, const char *name);
  inode_t* (*find)(inode_t* cur,const char* path,int flags);
};

#define DIREC 0
#define FIL 1
struct inode {
  int type;
  int refcnt;
  void* ptr;
  filesystem_t *fs;
  inodeops_t *ops; 
};

struct file {
  int flags;
  int refcnt; 
  inode_t *inode;
  uint32_t offset;
  spinlock_t lk;
};

typedef struct{
  void (*init)();
  int (*access)(const char *path, int mode);
  int (*mount)(const char *path, filesystem_t *fs);
  int (*unmount)(const char *path);
  int (*mkdir)(const char *path);
  int (*rmdir)(const char *path);
  int (*link)(const char *oldpath, const char *newpath);
  int (*unlink)(const char *path);
  int (*open)(const char *path, int flags);
  ssize_t (*read)(int fd, void *buf, size_t nbyte);
  ssize_t (*write)(int fd, void *buf, size_t nbyte);
  off_t (*lseek)(int fd, off_t offset, int whence);
  int (*close)(int fd);
} MODULE(vfs);

#endif

/*
typedef struct directory* directory_ptr;
struct directory{
    char name[32];
    inode_t* ind;
    directory_ptr next;
};
typedef struct directory directory_t;
*/