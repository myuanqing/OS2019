#include <devices.h>
#include <common.h>
#include <vfs.h>

extern const device_t *devices[];
extern const size_t devices_cnt;

static inodeops_t devfs_iops,devfs_rootiops;

static void devfs_init(filesystem_t* fs,const char* name,device_t *dev){
    fs->name  =name;
    fs->dev   =dev;
    fs->inodes=pmm->alloc(sizeof(inode_t)*devices_cnt);

    fs->root  =pmm->alloc(sizeof(inode_t));
    *fs->root =(inode_t){
            .ptr=NULL,
            .fs =fs,
            .ops=&devfs_rootiops,
        };

    for(int i=0;i<devices_cnt;++i){
        fs->inodes[i].ptr=(void*)devices[i];
        fs->inodes[i].fs=fs;
        fs->inodes[i].ops=&devfs_iops;
    }
}

static inode_t* devfs_lookup(filesystem_t* fs,const char* path,int flags){
    if((!path[0]))return devfs.root;
    return vfs_find(devfs.root,path,flags);
}

static int devfs_close(inode_t* inode){
    return 0;
}

static fsops_t devfs_ops={
    .init  =devfs_init,
    .lookup=devfs_lookup,
    .close =devfs_close,
};

static device_t* get_dev(file_t* file){
    return (device_t*)file->inode->ptr;
}


static int devfs_iopen(file_t* file,int flags){
    file->offset=0;
    file->flags=flags;
    file->refcnt=1;
    kmt->spin_init(&file->lk,"");
    return 0;
}

static int devfs_iclose(file_t* file){
    kmt->spin_lock(&file->lk);
    --file->refcnt;
    if(file->refcnt==0){
        return file->inode->fs->ops->close(file->inode);
    }
    kmt->spin_unlock(&file->lk);
    return 0;
}

static ssize_t devfs_iread(file_t* file,char* buf,size_t size){
    device_t* dev=get_dev(file);
    ssize_t ret = dev->ops->read(dev,file->offset,buf,size);
    file->offset+=ret;
    return ret;
}

static ssize_t devfs_rootiread(file_t* file,char* buf,size_t size){
    return EISDIR;
}

static ssize_t devfs_iwrite(file_t* file,const char* buf,size_t size){
    device_t* dev=get_dev(file);
    ssize_t ret  =dev->ops->write(dev,file->offset,buf,size);
    file->offset+=ret;
    return ret;
}

static ssize_t devfs_rootiwrite(file_t* file,const char* buf,size_t size){
    return EISDIR;
}


static ssize_t devfs_ilseek(file_t* file,off_t offset,int whence){
    switch(whence){
        case SEEK_SET:
            return file->offset=offset;
        case SEEK_CUR:
            return file->offset+=offset;
        case SEEK_END:
            TODO();
    }
    BARRIER();
}

static ssize_t devfs_imkdir(inode_t* inode,const char* name){
    fprintf(STDERR,"cannot create directory '%s': Read-only filesystem\n",name);
    return -1;
}

static ssize_t devfs_irmdir(inode_t* inode,const char* name){
    fprintf(STDERR,"cannot remove directory '%s': Read-only filesystem\n",name);
    return -1;
}


static inode_t* devfs_ifind(inode_t* cur,const char* path,int flags){
    if(*path||(flags&O_DIRECTORY)){
        while(*path=='/')++path;
        if(*path){
            //warn("No such file or directory");
        }else{
            //warn("Not a directory");
        }
    }else{
        return cur;
    }
    return NULL;
}

static inode_t* devfs_rootifind(inode_t* cur,const char* path,int flags){
    inode_t* next=NULL;
    //printf("Here\n");
    while(*path=='/')++path;
    if(!*path)return cur;

    if(path[0]=='.'){
        if(path[1]=='.'){
            //.. for parent
            next=devfs.root_parent;
            path+=2;
        }else{
            //. for current
            next=cur;
            ++path;
        }
    }else{
        for(int i=0;i<devices_cnt;++i){
            int len=strlen(devices[i]->name);
            if( strncmp(devices[i]->name, path,len)==0&&
                (path[len]=='/' || path[len]=='\0') ){

                next=devfs.inodes+i;
                path+=len;
            }
        }
        if(!next){
            //warn("No such file or directory");
            return NULL;
        }
    }

    return vfs_find(next,path,flags);
}


static inodeops_t devfs_iops={
    .open   =devfs_iopen,
    .close  =devfs_iclose,
    .read   =devfs_iread,
    .write  =devfs_iwrite,
    .lseek  =devfs_ilseek,
    .mkdir  =devfs_imkdir,
    .rmdir  =devfs_irmdir,
    .find   =devfs_ifind,
};

static inodeops_t devfs_rootiops={
    .open   =devfs_iopen,
    .close  =devfs_iclose,
    .read   =devfs_rootiread,
    .write  =devfs_rootiwrite,
    .lseek  =devfs_ilseek,
    .mkdir  =devfs_imkdir,
    .rmdir  =devfs_irmdir,
    .find   =devfs_rootifind,
};



filesystem_t devfs={
    .ops     =&devfs_ops,
    .dev     =NULL,
};

int isatty(int fd){
    task_t* cur=get_cur();
    device_t*  dev=get_dev(cur->fd[fd]);
    extern devops_t tty_ops;
    return dev->ops==&tty_ops;
}
