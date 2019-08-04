#include <common.h>
#include <devices.h>
#include <vfs.h>
#include <pathread.h>

extern int current_id[MAX_CPU];
extern task_entry tasks[MAX_TASK];

#define this_fd (current->fd[fd])


int fprintf(int fd,const char* fmt, ...){
    char buf[0x100];
    va_list ap;
    va_start(ap,fmt);
    int len=vsprintf(buf,fmt,ap);//vsprintf will call va_end(ap);
    return vfs->write(fd,buf,len);
}

struct{
    const char* path;
    inode_t backup;
}mount_table[20];
#define mtt mount_table

int mount_table_cnt=0;
#define mtt_tab mtt[mount_table_cnt]


spinlock_t mount_table_lk;

int new_fd_num(task_t* current){
    for(int i=0;i<NOFILE;++i){
        if(!(current->fd[i])){
            return i;
        }
    }
    return -1;//No more file descripter (number)!
}
static inode_t* vfs_root=NULL;

inode_t* vfs_lookup(const char* path,int flags);
inode_t* vfs_lookup(const char* path,int flags){
    inode_t* start=NULL;
    if(*path=='/'){
        start=vfs_root;
        //printf("%p\n",vfs_root);
    }else{
        start=get_cur()->cur_dir;
    }
    return start->ops->find(start,path,flags);
}

void vfs_init(void){
    kmt->spin_init(&mount_table_lk,"");
    blkfs.ops->init  (&blkfs    ,"ramdisk1" ,dev_lookup("ramdisk1") );
    devfs.ops->init     (&devfs     ,"devfs"    ,NULL                   );
//    procfs.ops->init    (&procfs    ,"procfs"   ,NULL                   );

    vfs->mount("/"      ,&blkfs);
    vfs->mount("/dev/"   ,&devfs);   //printf("here\n");
//    vfs->mount("/proc/"  ,&procfs);
}



int vfs_access(const char *path, int mode){
    return vfs_lookup(path,mode)==NULL;
}


int vfs_mount(const char *path, filesystem_t *fs){
    kmt->spin_lock(&mount_table_lk);

    if(strcmp(path,"/")){
        inode_t* origin=vfs_lookup(path,O_RDONLY|O_DIRECTORY);
        //printf("%s\n",path);
        //Replace origin inode at path
        if(origin==NULL){
            TODO();
        }
        mtt_tab=( typeof(mtt_tab) ){
            .path=path,
            .backup=*origin,
        };
        *origin =*fs->root;
        ++mount_table_cnt;

        char root_parent[0x100];
        strcpy(root_parent,path);
        dir_cat(root_parent,"..");
        fs->root_parent=vfs_lookup(root_parent,O_RDONLY|O_DIRECTORY);
    }else{
        vfs_root=fs->root_parent=fs->root;
    }

    kmt->spin_unlock(&mount_table_lk);
    return 0;
}
int vfs_unmount_real(const char *path){
    for(int i=0;i<mount_table_cnt;++i){
        if(!strcmp(path,mount_table[i].path)){
            --mount_table_cnt;
            *vfs_lookup(path,O_RDONLY|O_DIRECTORY)=mount_table[i].backup;
            mount_table[i]=mount_table[mount_table_cnt];
            return 0;
        }
    }
    return -1;
}

int vfs_unmount(const char* path){
    kmt->spin_lock(&mount_table_lk);
    int ret=vfs_unmount_real(path);
    kmt->spin_unlock(&mount_table_lk);
    return ret;
}




int vfs_mkdir(const char* path){
    if(vfs_lookup(path,O_RDONLY)){
        //warn("cannot create directory '%s': File exists",path);
        return -1;
    }else{
        //clear_warn();
        return vfs_lookup(path,O_RDONLY|O_CREATE|O_DIRECTORY)==NULL;
    }
}



static inline inode_t* get_parent(const char**s,char* new_parent){
    const char* path=*s;
    int len=get_last_slash(path)+1;
    *s=path+len;
    if(len==0){
        return get_cur()->cur_dir;
    }else{
        strncpy(new_parent,path,len);
        return vfs_lookup(new_parent,O_RDONLY);
    }
}


int vfs_rmdir(const char *path){
    char new_parent[0x100];

    inode_t* parent=get_parent(&path,new_parent);

    if(!parent){
        //warn("No such file or directory",new_parent);
    }else if(parent->ops->find(parent,path,O_RDONLY)){
        return parent->ops->rmdir(parent,path);
    }else{
        //warn("No such file or directory",new_parent);
    }
    return 0;
}

static inline int vfs_open_real(const char *path,int flags){
    task_t* current=get_cur();

    int fd=new_fd_num(current);
    assert(fd!=-1);//,"No more file descripter!");//Or return -1;

    inode_t* inode=vfs_lookup(path,flags);
    if(!inode)return -1;
    this_fd=pmm->alloc(sizeof(file_t));

    this_fd->inode=inode;
    this_fd->inode->ops->open(this_fd,flags);
    return fd;
}

static int vfs_open(const char *path, int flags){
    int ret=vfs_open_real(path,flags);
    return ret;
}


static inline ssize_t vfs_read_real(int fd, void* buf,size_t nbyte){
    task_t* current=get_cur();
    if(this_fd){
        return this_fd->inode->ops->read(this_fd,buf,nbyte);;
    }else{
        return 0;
    }
}

static ssize_t vfs_read(int fd,void *buf,size_t nbyte){
    ssize_t ret=vfs_read_real(fd,buf,nbyte);
    return ret;
}

static inline ssize_t vfs_write_real(int fd,void *buf,size_t nbyte){
    task_t* current=get_cur();
    if(this_fd){
        return this_fd->inode->ops->write(this_fd,buf,nbyte);
    }else{
        return 0;
    }
}

static ssize_t vfs_write(int fd,void* buf,size_t nbyte){
    ssize_t ret=vfs_write_real(fd,buf,nbyte);
    return ret;
}

off_t vfs_lseek(int fd,off_t offset, int whence){
    TODO();
}

int vfs_close(int fd){
    task_t* current=get_cur();
    this_fd->inode->ops->close(this_fd);
    assert(this_fd->refcnt>=0);//,"fd with refcnt <0!\n");
    if(this_fd->refcnt==0){
        pmm->free(this_fd);
    }
    this_fd=NULL;
    return 0;
}

MODULE_DEF(vfs){
  .init     =vfs_init,
  .access   =vfs_access,
  .mount    =vfs_mount,
  .unmount  =vfs_unmount,
  .mkdir    =vfs_mkdir,
  .rmdir    =vfs_rmdir,
  .open     =vfs_open,
  .read     =vfs_read,
  .write    =vfs_write,
  .lseek    =vfs_lseek,
  .close    =vfs_close,
};

ssize_t std_read(void *buf){
    return vfs->read(STDIN,buf,-1);
}
ssize_t std_write(void *buf){
    return vfs->write(STDOUT,buf,strlen(buf));
}