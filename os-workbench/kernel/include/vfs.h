#ifndef __VFS_H__
#define __VFS_H__

#define STDIN  0
#define STDOUT 1
#define STDERR 2

#define O_RDONLY    0x01
#define O_WRONLY    0x02
#define O_CREATE    0x04
#define O_DIRECTORY 0x08
#define O_APPEND    0x10

#define O_RDWR (O_RDONLY | O_WRONLY)


#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define EISDIR -1
#define ENOENT -1


int is_dir(inode_t*);

//check may call return, so I prefer write it as a macro
#define check(inode,path,flags) \
    do{ \
        if(!inode){ \
            return NULL; \
        } \
        if(*path=='/'){ \
            while(*path=='/')++path; \
            if(!is_dir(inode)){ \
                return NULL; \
            } \
        } \
        if(!*path){ \
            if((flags & O_DIRECTORY) && !is_dir(inode)){ \
                return NULL; \
            } \
            else return (inode_t*)inode; \
        } \
    }while(0);


extern filesystem_t
    blkfs,//Ramdisk
    devfs,
    procfs;

int fprintf(int fd,const char* fmt, ...);

static inline inode_t* vfs_find(inode_t* inode,const char* path,int flags){
    return inode->ops->find(inode,path,flags);
}
#endif

