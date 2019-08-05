#include"blkfs.h"
#include"pathread.h"
// 4 << 20 byte   4<<10 /block  
#define INODE_START 0x40
#define INODE_END 0x400

#define DIR_MODE (0)
#define FILE_MODE (1)
#define DISK_WIPE (0x3f3f3f3f)

 
// *************************************block*********************************************//

int block_cmp(device_t* dev,uint32_t off,const char* s){
    char buf[0x80];
    ssize_t (*const read)(device_t* dev,off_t offset,void* buf,size_t count)=dev->ops->read;

    read(dev,off+4,&buf,80);
    int ret=strncmp(buf,s,BLK_SZ-8);
    if(ret)return ret;

    read(dev,off+BLK_DT,&off,4);
    while(off){
        read(dev,off,buf,BLK_DT);
        strncmp(buf,s,BLK_DT);
        read(dev,off+BLK_DT,&off,4);
    }
    return 0;
}

uint32_t new_block(device_t* dev){
    uint8_t avail[0x40];
    dev->ops->read(dev,0,avail,0x40);
    for(int i=0;i<0x40;++i){
        if(avail[i]==0xff)continue;
        for(int j=0;j<8;++j){
            if(avail[i]&(1<<j)){
                continue;
            }
            uint32_t ret=(8*i+j)*0x80;
            avail[i]|=1<<j;
            dev->ops->write(dev,i,avail+i,1);
            return ret;
        }
    }
    return 0;
}

int block_find(device_t* dev,uint32_t* fd_off,uint32_t* off){
    for(;*fd_off>BLK_DT;*fd_off-=BLK_DT){
        if(dev->ops->read(dev,*off+BLK_DT,off,4)!=4)return -1;
    }
    return 0;
}

int block_write(device_t* dev,uint32_t off,uint32_t shift,const char* s,size_t nbyte){
    ssize_t (*const read)(device_t* dev,off_t offset,void* buf,size_t count)=dev->ops->read;
    ssize_t (*const write)(device_t* dev,off_t offset,const void* buf,size_t count)=dev->ops->write;
    size_t rest=nbyte;
    block_find(dev,&shift,&off);
    off+=shift;
    {
        int to_write=min(BLK_DT-(off)%BLK_SZ,rest);
        if(write(dev,off,s,to_write)!=to_write){
            return nbyte-rest;
        }
        s   +=to_write;
        rest-=to_write;
        off +=to_write;
    }
    while(rest>0){
        uint32_t new_off=0;
        if(read(dev,off,&new_off,4)!=4||!new_off){
            new_off=new_block(dev);
            write(dev,off,&new_off,4);
            if(off==0){
                fprintf(2,"No more space on this disk!\n");
                return nbyte-rest;
            }
        }
        off=new_off;
        int to_write=min(BLK_DT,rest);
        if(write(dev,off,s,to_write)!=to_write){
            return nbyte-rest;
        }
        s+=to_write;
        rest-=to_write;
        off+=to_write;
    }
    return nbyte;
}

uint32_t new_inode(device_t* dev){
    uint32_t off;
    uint32_t buf;
    for(off=INODE_START;;off+=sizeof(disknode_t)){
        dev->ops->read(dev,off,&buf,0x4);
        if(buf==0){
            uint32_t one=1;
            dev->ops->write(dev,off,&one,0x4);
            return off;
        }
    }
}




int block_read (device_t* dev,uint32_t off,uint32_t shift,char* s,size_t nbyte){
    ssize_t (*const read)(device_t* dev,off_t offset,void* buf,size_t count)=dev->ops->read;
    size_t rest=nbyte;
    block_find(dev,&shift,&off);
    off+=shift;
    while(rest>0){
        int to_read=min(BLK_DT-(off)%BLK_SZ,rest);
        if(read(dev,off,s,to_read)!=to_read){
            return nbyte-rest;
        }
        s+=to_read;
        rest-=to_read;
        if(read(dev,off+BLK_DT,&off,4)!=4||!off){
            return nbyte-rest;
        };
    }
    return nbyte;
}


static inline ssize_t write(device_t* dev,off_t offset,uint32_t to_write,size_t count){
    uint32_t tmp=to_write;
    return dev->ops->write(dev,offset,&tmp,4);
}

int disk_init(device_t* dev){

    uint8_t bitmap[0x40]={0xff};

    disknode_t root_inode={
        .refcnt=1,
        .info=0x480,
        .size=8,
        .type=DIR_MODE,
    };
    block_t root_name={
        .mem="/",
        .next=0,
    };
    dev->ops->write(dev,0x040,&root_inode,sizeof(root_inode));
    dev->ops->write(dev,0x400,&root_name,sizeof(root_name));

    struct pair{
        uint32_t addr, info;
    }pairs[]={
        /*  /   */
        {0x040,0x00000001}, //refcnt
        {0x044,0x00000480}, //info
        {0x048,0x00000008}, //size
        {0x04c,0x00000000}, //type

        /* /dev  */
        {0x050,0x00000001},
        {0x054,0x00000580},
        {0x058,0x00000000},
        {0x05c,0x00000000},

        /* /proc */
        {0x060,0x00000001},
        {0x064,0x00000680},
        {0x068,0x00000000},
        {0x06c,0x00000000},

    /*  information */
        /*  /   */
        {0x400,0x00000000},
        {0x404,       '/'},
        {0x480,0x00000000},
        {0x484,0x00000500},
        {0x488,0x00000600},

        /*  /dev   */
        {0x500,0x00000001},
        {0x504,0x00766564},
        {0x580,0x00000000},
        

        /*  /proc  */
        {0x600,0x00000002},
        {0x604,0x636f7270},
        {0x680,0x00000000},
    };


    for(int i = 0; i< sizeof(pairs)/sizeof(pairs[0]);i++){
        dev->ops->write(dev,pairs[i].addr, &pairs[i].info, sizeof(pairs[0].info));        
    }

    //rd_t* rd=dev->ptr;
    
    //uint32_t id=(rd->end-rd->start)/0x80;
    for(int i=0;i<0x40;++i){
        bitmap[i]=0xff;
    }

    dev->ops->write(dev,0,bitmap,sizeof(bitmap));
    return 0;
}
//  ************************************fs************************************************//
static inodeops_t blkfs_iops;
static void blkfs_init(filesystem_t* fs,const char* name,device_t* dev){
    fs->name  =name;
    fs->dev   =dev;
    fs->name  =name;
    fs->dev   =dev;
    fs->inodes=pmm->alloc(sizeof(inode_t)*(INODE_END-INODE_START)/sizeof(disknode_t));
    fs->root  =fs->inodes;

    uint32_t check=0;
    dev->ops->read(dev,0,&check,4);
    if((check&0xff)<0xff){
        //Empty ramdisk
        disk_init(dev);
    }
   
    for(int i=0;INODE_START+i*sizeof(disknode_t)<INODE_END;++i){
        fs->inodes[i].ptr=pmm->alloc(sizeof(disknode_t));
        fs->dev->ops->read(fs->dev,
                INODE_START+i*sizeof(disknode_t),
                fs->inodes[i].ptr,
                sizeof(disknode_t) );
        fs->inodes[i].fs=fs;
        fs->inodes[i].ops=&blkfs_iops;
    }
}

static inode_t* blkfs_lookup(filesystem_t* fs,const char* path,int flags){
    if((!path[0]))return fs->root;
    return vfs_find(fs->root,path,flags);
}


static int blkfs_close(inode_t* inode){
    return 0;
}


static fsops_t blkfs_ops={
    .init = blkfs_init,
    .lookup = blkfs_lookup,
    .close = blkfs_close,
};





//*********************************************************inode******************************************************/

static inline void update_size(file_t* file,uint32_t newsize){
    filesystem_t* fs = file->inode->fs;
    device_t* dev  = fs->dev;
    uint32_t  id   = file->inode - fs->inodes;
    ((disknode_t*)file->inode->ptr)->size=newsize;

    dev->ops->write(dev,INODE_START+id*sizeof(disknode_t)+offsetof(disknode_t,size),&newsize,4);
}

static int blkfs_iopen(file_t* file,int flags){
    kmt->spin_init(&file->lk, "");
    kmt->spin_lock(&file->lk);
    if(((disknode_t*)file->inode->ptr)->type==DIR_MODE){
        //First 4 bytes indicates the parent directory in my file system
        file->offset=4;
    }else{
        file->offset=0;
        if(  (flags & O_WRONLY)&&
            !(flags & O_RDONLY)&&
            !(flags & O_APPEND) ){
            update_size(file,0);
        }
    }
    file->flags=flags;
    file->refcnt=1;
    kmt->spin_unlock(&file->lk);
    return 0;
}

static int blkfs_iclose(file_t* file){
    kmt->spin_init(&file->lk,"");
    --file->refcnt;
    if(file->refcnt==0){
        return file->inode->fs->ops->close(file->inode);
    }
    kmt->spin_unlock(&file->lk);
    return 0;
}

static ssize_t inline blkfs_iread(file_t* file,char* buf,size_t size){
    filesystem_t* fs  = file->inode->fs;
    uint32_t fd_off = file->offset;
    disknode_t* node  = file->inode->ptr;
    uint32_t off    = node->info;
    uint32_t fsize  = node->size;

    switch(node->type){
        case DIR_MODE:
            return EISDIR;
            break;
        case FILE_MODE:
            {
                if(fd_off>size){
                    return 0;
                }else if(size+fd_off>fsize){
                    size=fsize-fd_off;
                }
                ssize_t nread=block_read(fs->dev,off,fd_off,buf,size);
                file->offset+=nread;
                return nread;
            }
            break;
    }
    return 0;
}


static ssize_t blkfs_iwrite(file_t* file,const char* buf,size_t size){
    filesystem_t* fs  = file->inode->fs;
    uint32_t fd_off;
    if(file->flags&O_APPEND){
        fd_off = file->inode->ops->lseek(file,0,SEEK_END);
    }else{
        fd_off = file->offset;
    }
    disknode_t* node  = file->inode->ptr;
    uint32_t off    = node->info;
    uint32_t fsize  = node->size;

    if(off==0&&fsize==0){
        node->info=off=new_block(fs->dev);
    }
    switch(node->type){
        case DIR_MODE:
            return EISDIR;
            break;
        case FILE_MODE:
            {
                ssize_t nwrite=block_write(fs->dev,off,fd_off,buf,size);
                file->offset+=nwrite;
                if(file->offset>fsize){
                    update_size(file,file->offset);
                }
                return nwrite;
            }
            break;
        default:
            TODO();
    }
    assert(0);
}


static off_t blkfs_ilseek(file_t* file,off_t offset,int whence){
    switch(whence){
        case SEEK_SET:
            return file->offset=offset;
        case SEEK_CUR:
            return file->offset+=offset;
        case SEEK_END:
            {
                disknode_t* node  = file->inode->ptr;
                uint32_t fsize  = node->size;
                return file->offset=(offset+fsize);
            }
    }
    BARRIER();
}

static int blkfs_irmdir(inode_t* parent,const char* name){
    const filesystem_t* fs=parent->fs;
    device_t* dev=fs->dev;
    disknode_t* node=parent->ptr;
    uint32_t offset=node->info;//Skip parent

    ssize_t(*const read )(device_t*,off_t,      void*,size_t)=dev->ops->read;
    ssize_t(*const write)(device_t*,off_t,const void*,size_t)=fs->dev->ops->write;

    if(name[0]=='.'){
        return -1;
    }else{
        offset+=4;
        while(offset){
            uint32_t blk_off;
            if(read(dev,offset,&blk_off,4)!=4||!blk_off){
                //No more file in this directory
                
                return -1;
            }
            if(blk_off==DISK_WIPE||block_cmp(dev,blk_off,name)){
                //Mismatch
                offset+=4;
                if(offset%BLK_SZ==BLK_DT){
                    //Find next offset
                    read(dev,offset,&offset,4);
                }
            }else{
                uint32_t id,type;
                read(dev,blk_off,&id,4);
                read(dev,INODE_START+id*sizeof(disknode_t)+offsetof(disknode_t,type),&type,4);
                if(type!=DIR_MODE){
                    return -1;
                }else{
                    //TODO: refcnt
                    uint32_t wipe=DISK_WIPE;
                    write(dev,offset,&wipe,4);
                    return 0;
                }
            }
        }
    }

    return -1;
}

static inline uint32_t get_id(const inode_t* cur){
    const filesystem_t* fs=cur->fs;
    uint32_t ret=cur-fs->inodes;
    assert(fs->inodes+ret==cur);
    return ret;
}

static inline void add_inode(const filesystem_t* fs,const uint32_t id,const disknode_t* node){
    *(disknode_t*)fs->inodes[id].ptr=*node;
    device_t* dev=fs->dev;
    dev->ops->write(dev,INODE_START+id*sizeof(*node),node,sizeof(*node));
}


static inline inode_t* new_direc(
        const inode_t* cur,uint32_t offset,const char* filename,int flags){
    const filesystem_t* fs=cur->fs;
    device_t* dev=fs->dev;

    ssize_t(*const write)(device_t*,off_t,const void*,size_t)=fs->dev->ops->write;

    uint32_t off=new_block(dev),inode=new_inode(dev);
    disknode_t file={
        .refcnt=1,
        .info  =new_block(dev),
        .size  =0,
        .type  =DIR_MODE,
    };
    write(dev,offset,&off,4);

    int id=(inode-INODE_START)/sizeof(disknode_t);
    add_inode(fs,id,&file);

    write(dev,off,&id,4);
    write(dev,off+4,filename,strlen(filename));
    uint32_t parent_id=get_id(cur);
    write(dev,file.info,&parent_id,4);
    return fs->inodes+id;
}

static inline inode_t* new_file(
        const inode_t* cur,uint32_t offset,const char* filename,int flags){
    const filesystem_t* fs=cur->fs;
    device_t* dev=fs->dev;

    ssize_t(*const write)(device_t*,off_t,const void*,size_t)=fs->dev->ops->write;

    uint32_t off=new_block(dev),inode=new_inode(dev);
    disknode_t file={
        .refcnt=1,
        .info  =new_block(dev),
        .size  =0,
        .type  =FILE_MODE,
    };
    write(dev,offset,&off,4);

    int id=(inode-INODE_START)/0x10;
    add_inode(fs,id,&file);

    write(dev,off,&id,4);
    write(dev,off+4,filename,strlen(filename));
    return fs->inodes+id;
}

int is_dir(inode_t* inode){
    disknode_t* node=inode->ptr;
    return node->type==DIR_MODE;
}

static inode_t* blkfs_ifind(inode_t* cur,const char* path,int flags){
    inode_t* next=NULL;
    //printf("$$%s\n",path);
    if(!cur){
        //printf("p\n");
        return NULL; 
    } 
    if(*path=='/'){ 
        while(*path=='/')++path; 
        if(!is_dir(cur)){ 
            //printf("p\n");
            return NULL; 
        } 
    } 
    if(!*path){ 
        if((flags & O_DIRECTORY) && !is_dir(cur)){ 
            //printf("p\n");
            return NULL; 
        } 
        else {
            //printf("p\n");
            return (inode_t*)cur;
        }
    } 
    
    //printf("here\n");
    const filesystem_t* fs=cur->fs;
    device_t* dev=fs->dev;
    disknode_t* node=cur->ptr;
    uint32_t offset=node->info;//Skip parent
    //printf("%x\n",offset);
    ssize_t(*const read )(device_t*,off_t,      void*,size_t)=fs->dev->ops->read;

    uint32_t id=-1;//id of inode

    if(path[0]=='.'){
        if(path[1]=='.'){
            //.. for parent
            if(cur==cur->fs->root){
                next=cur->fs->root_parent;
            }else{
                read(dev,offset,&id,4);
                next=fs->inodes+id;
            }
            path+=2;
        }else{
            //. for current
            next=cur;
            ++path;
        }
    }else{
       
        offset+=4;
        char layer[0x100];
        int layer_len=get_first_layer(path);
        //printf("layer_len:%d\n",layer_len);
        strncpy(layer,path,layer_len);
        //printf("%s\n",layer);
        while(offset){
            uint32_t blk_off;
            if(read(fs->dev,offset,&blk_off,4)!=4||!blk_off){
                //No more file in this directory
                //printf("%x\n", blk_off);
                if( (flags&O_CREATE) && (path[layer_len]=='\0')){
                    if(flags&O_DIRECTORY)//Only mkdir will gets here
                        return new_direc(cur,offset,path,flags);
                    else
                        return new_file (cur,offset,path,flags);
                }else{
                    //printf("here\n");
                    return NULL;
                }
            }
            if(blk_off==DISK_WIPE||block_cmp(fs->dev,blk_off,layer)){
                //Mismatch
                offset+=4;
                if(offset%BLK_SZ==BLK_DT){
                    //Find next offset
                    read(fs->dev,offset,&offset,4);
                }
            }else{
                path+=layer_len;
                read(fs->dev,blk_off,&id,4);
                next=fs->inodes+id;
                break;
            }
        }
    }

    return next->ops->find(next,path,flags);
}

static inodeops_t blkfs_iops={
    .open   =blkfs_iopen,
    .close  =blkfs_iclose,
    .read   =blkfs_iread,
    .write  =blkfs_iwrite,
    .lseek  =blkfs_ilseek,
    /*
    .mkdir  =blkfs_imkdir,
    */
    .rmdir  =blkfs_irmdir,
    .find   =blkfs_ifind,
};


filesystem_t blkfs={
    .name = NULL,
    .ops = &blkfs_ops,
};