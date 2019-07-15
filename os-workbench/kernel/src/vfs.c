#include<common.h>
#include<klib.h>
#include<am.h>
#include<devices.h>

inode_t* root_inode;
spinlock_t root_lk;
spinlock_t tty_lk[4];


typedef struct directory* directory_ptr;
struct directory{
    char name[32];
    inode_t* ind;
    directory_ptr next;
};
typedef struct directory directory_t;


static void vfs_init();
static int vfs_mount(const char *path, filesystem_t *fs);
static int vfs_unmount(const char*path);


static ssize_t tty_write(file_t *file, const char*buf, size_t size){
    inode_t* ind = file->inode;
    device_t* tty = (device_t*)ind->ptr;
    tty->ops->write(tty,0,buf,size);
    return size;
              
}

static ssize_t ramdisk_write(file_t *file, const char*buf, size_t size){
    inode_t* ind = file->inode;
    device_t* ramdisk = (device_t*)ind->ptr;
    ramdisk->ops->write(ramdisk,0,buf,size);
    return size;
}

static ssize_t null_write(file_t *file, const char*buf, size_t size){
    return 0;
}
static ssize_t null_read(file_t *file, char*buf, size_t size){
    return 0;
}

static ssize_t zero_write(file_t *file, const char*buf, size_t size){
    return 0;
}
static ssize_t zero_read(file_t *file, char*buf, size_t size){
    memset(buf, '\0', size);
    return size;    
}

static ssize_t random_write(file_t *file, const char*buf, size_t size){
    return 0;
}
static ssize_t random_read(file_t *file, char*buf, size_t size){
    int c = rand(); 
    memcpy(buf, &c,sizeof(int));
    return size;    
}

//static ssize_t tty_read(file_t *file, char *buf, size_t size){
//    inode_t* ind = file->inode;
//    device_t* tty = (device_t*)ind->ptr;
//    tty->ops->read(tty,0,buf,size);
//    return size;
//}
static inode_t* blkfs_lookup(inode_t* ind, const char* name, int flags){
    int i = 1;
    int begin = 1;
    if(ind != root_inode){
       i=0;
       begin = 0;
    }
    while(name[i] != '\0'){    
        if(name[i] == '/'){
            //printf("%d\n",i);
            //inode_t* next= NULL;
            if(ind->type == DIREC){
                directory_ptr ptr = (directory_ptr)ind->ptr;
                while(ptr!=NULL){
                    //printf("%s,%s\n",name+begin,ptr->name);
                    //printf("%d\n",i-begin);
                    if(strncmp(ptr->name, name+begin, i-begin)==0){
                        //printf("here\n");
                        return ptr->ind->fs->ops->lookup(ptr->ind, name+i+1, flags);                                
                    }else{
                        ptr = ptr->next;
                    } 
                }
                return NULL;
            }else{
                return NULL;    
            }
        }
        i++;   
    }
    if( ind->type == DIREC ){
        if(i == 0){
            return ind;
        }
        directory_ptr ptr = (directory_ptr)ind->ptr;  
        while(ptr!=NULL){
            if(strncmp(ptr->name, name, i)==0){
                
                return ptr->ind;                                
            }else{
                ptr = ptr->next;
            } 
        }
        return NULL;
    }else if(ind->type == FIL){
//        printf("here\n");
        if(i == 0){return NULL;}
        return ind;
    }
    return NULL;
}

static inode_t* devfs_lookup(inode_t* ind, const char* name, int flags){
//    printf("lookup%p\n",ind);
    int i = 0;
//    printf("begin\n");
    while(name[i] != '\0'){
        if(name[i] == '/'){
            //inode_t* next= NULL;
            if(ind->type == DIREC){
                directory_ptr ptr = (directory_ptr)ind->ptr;
                while(ptr!=NULL){
                    if(strncmp(ptr->name, name, i)==0){
                        return ptr->ind->fs->ops->lookup(ptr->ind, name+i+1, flags);                                
                    }else{
                        ptr = ptr->next;
                    } 
                }
                return NULL;
            }else{
                return NULL;    
            }
        }
        i++;    
    }
    if( ind->type == DIREC ){
        if(i == 0){
            return ind;
        }
        directory_ptr ptr = (directory_ptr)ind->ptr;  
        while(ptr!=NULL){
            if(strncmp(ptr->name, name, i)==0){
                
                return ptr->ind;                                
            }else{
                ptr = ptr->next;
            } 
        }
        return NULL;
    }else if(ind->type == FIL){
//        printf("here\n");
        if(i == 0){return NULL;}
        return ind;
    }
    return NULL;
}




static void mount_blkfs(filesystem_t* fs){
    fs->ops = pmm->alloc(sizeof(fsops_t));
    fs->dev = NULL;
    fs->ops->init = NULL;
    fs->ops->close = NULL;
    fs->ops->lookup = blkfs_lookup;
}

static void mount_devfs(filesystem_t* fs){
    fs->ops = pmm->alloc(sizeof(fsops_t));
    fs->dev = NULL;
    fs->ops->init = NULL;
    fs->ops->close = NULL;
    fs->ops->lookup = devfs_lookup;
}   




static void vfs_init(){
    kmt->spin_init(&root_lk,"rootfs");
    for(int i = 0;i < 4;++i){
        kmt->spin_init(&tty_lk[i], "tty");
    }
    root_inode = NULL;
    filesystem_t *blkfs = pmm->alloc(sizeof(filesystem_t));
    mount_blkfs(blkfs);
    vfs_mount("/",blkfs);
    filesystem_t *devfs = pmm->alloc(sizeof(filesystem_t));
    mount_devfs(devfs);
    vfs_mount("/dev",devfs);
    inode_t* ind = root_inode->fs->ops->lookup(root_inode,"/dev/tty1",1);
    file_t* filep = pmm->alloc(sizeof(file_t));
    filep->inode = ind;
    if(ind!=NULL){
        ind->ops->write(filep, "1234",5);
        //char buf[10];
        //ind->ops->read(filep,buf,3);
        //printf("%s\n",buf);
    }
    //if(devfs_lookup)
    //devfs->init();    
}

static int vfs_mount(const char *path, filesystem_t *fs){
    if(strcmp(path,"/")==0){
        kmt->spin_lock(&root_lk);
        root_inode = (inode_t*)pmm->alloc(sizeof(inode_t));
        root_inode->refcnt = 0;
        //root_inode->ptr = NULL;
        root_inode->fs = fs;
        root_inode->ops = NULL;
        root_inode->type = DIREC;

        root_inode->ptr = pmm->alloc(sizeof(directory_t));
        directory_ptr ptr = (directory_ptr)root_inode->ptr;
        memcpy(ptr->name, ".",2);
        ptr->ind = root_inode;
        ptr->next = (directory_ptr)pmm->alloc(sizeof(directory_t));
        memcpy(ptr->next->name, "..",3);
        ptr->next->ind = root_inode; 
        ptr->next->next = NULL;
        kmt->spin_unlock(&root_lk);
    }else {
        if(strcmp(path,"/dev")==0){
            kmt->spin_lock(&root_lk);
            inode_t* ind = (inode_t*)pmm->alloc(sizeof(inode_t));
            //printf("%p\n",ind);
            directory_ptr dptr = (directory_ptr)pmm->alloc(sizeof(directory_t));
            memcpy(dptr->name, "dev", 4);
            dptr->ind = ind;
            dptr->next = (directory_ptr)root_inode->ptr;
            root_inode->ptr = dptr;
            
            
            ind->refcnt = 0;
            ind->fs = fs;
            ind->ops = NULL;
            ind->type = DIREC;
            ind->ptr = pmm->alloc(sizeof(directory_t));
            directory_ptr ptr = (directory_ptr)ind->ptr;        
            memcpy(ptr->name, ".",2);
            ptr->ind = root_inode;
            ptr->next = (directory_ptr)pmm->alloc(sizeof(directory_t));
            memcpy(ptr->next->name, "..",3);
            ptr->next->ind = ind; 
            ptr->next->next = NULL;
            //if(ind->ptr == NULL){printf("hambu\n");}
            ptr->next->next = (directory_ptr)pmm->alloc(sizeof(directory_t));
            directory_ptr dev_ptr = ptr->next->next;
            const char tty [4][5]= {"tty1","tty2","tty3","tty4"};
            for(int i = 0;i < 4; ++i){
                inode_t* dev_ind = (inode_t*)(pmm->alloc(sizeof(inode_t)));
                memcpy(dev_ptr->name,tty[i],5);
                dev_ptr->ind = dev_ind;
                dev_ptr->next = (directory_ptr)pmm->alloc(sizeof(directory_t));
                dev_ptr = dev_ptr->next;
                dev_ind->type = FIL;
                dev_ind->refcnt = 0;
                dev_ind->ptr = dev_lookup(tty[i]);
                dev_ind->fs = fs;
                dev_ind->ops = pmm->alloc(sizeof(inodeops_t));
                dev_ind->ops->write = tty_write;
                //dev_ind->ops->read = tty_read;
            }
            const char ramdisk[2][9]={"ramdisk0","ramdisk1"};
            for(int i = 0;i < 2;++i){
                inode_t* dev_ind = (inode_t*)(pmm->alloc(sizeof(inode_t)));
                memcpy(dev_ptr->name,tty[i],9);
                dev_ptr->ind = dev_ind;
                dev_ptr->next = (directory_ptr)pmm->alloc(sizeof(directory_t));
                dev_ptr = dev_ptr->next;
                dev_ind->type = FIL;
                dev_ind->refcnt = 0;
                dev_ind->ptr = dev_lookup(ramdisk[i]);
                dev_ind->fs = fs;
                dev_ind->ops = pmm->alloc(sizeof(inodeops_t));
                dev_ind->ops->write = ramdisk_write;
            }

            inode_t* null_ind = (inode_t*)(pmm->alloc(sizeof(inode_t)));
            memcpy(dev_ptr->name,"null",5);
            dev_ptr->ind = null_ind;
            dev_ptr->next = (directory_ptr)pmm->alloc(sizeof(directory_t));
            dev_ptr = dev_ptr->next;
            null_ind->type = FIL;
            null_ind->refcnt = 0;
            null_ind->ptr = NULL;
            null_ind->fs = fs;
            null_ind->ops = pmm->alloc(sizeof(inodeops_t));
            null_ind->ops->read = null_read;
            null_ind->ops->write = null_write;

            inode_t* zero_ind = (inode_t*)(pmm->alloc(sizeof(inode_t)));
            memcpy(dev_ptr->name,"zero",5);
            dev_ptr->ind = zero_ind;
            dev_ptr->next = (directory_ptr)pmm->alloc(sizeof(directory_t));
            dev_ptr = dev_ptr->next;
            zero_ind->type = FIL;
            zero_ind->refcnt = 0;
            zero_ind->ptr = NULL;
            zero_ind->fs = fs;
            zero_ind->ops = pmm->alloc(sizeof(inodeops_t));
            zero_ind->ops->read = zero_read;
            zero_ind->ops->write = zero_write;


            inode_t* random_ind = (inode_t*)(pmm->alloc(sizeof(inode_t)));
            memcpy(dev_ptr->name,"random",5);
            dev_ptr->ind = random_ind;
            dev_ptr->next = (directory_ptr)pmm->alloc(sizeof(directory_t));
            dev_ptr = dev_ptr->next;
            random_ind->type = FIL;
            random_ind->refcnt = 0;
            random_ind->ptr = NULL;
            random_ind->fs = fs;
            random_ind->ops = pmm->alloc(sizeof(inodeops_t));
            random_ind->ops->read = random_read;
            random_ind->ops->write = random_write;
            //if(ind->fs->ops->lookup(ind,"tty1",1)!=NULL){
                //printf("hehe\n");
            //}            

            kmt->spin_unlock(&root_lk);
        }
        //if(root_inode == NULL ){
        //    return -1;
        //}else{
        //    inode_t* father_inode_ptr = root_inode->fs->ops->lookup(root_inode, path,0); 
        
        //    return 0;
        //}
    }    
    return -1;    
}

static int vfs_unmount(const char*path ){
    
    return 1;
}

MODULE_DEF(vfs){
	.init = vfs_init,
//	.access = vfs_access,
	.mount = vfs_mount,
	.unmount = vfs_unmount,
//	.open = vfs_open,
//	.read = vfs_read,
//	.write = vfs_write,
//	.lseek = vfs_lseek,
//	.close = vfs_close,
};