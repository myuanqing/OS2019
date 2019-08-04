#ifndef __BLKFS_H__
#define __BLKFS_H__

#include<common.h>
#include<klib.h>
#include<am.h>
#include<devices.h>

#define HEADER_OFF 0x40
#define INODE_START 0x40
#define INODE_END 0x400


typedef struct disknode disknode_t;
typedef struct disk_block block_t;

struct disknode{
    uint32_t refcnt;
    uint32_t info;
    uint32_t size;
    uint32_t type;
};
#define BLK_DT (0x80 - 4)
struct disk_block{
    uint8_t mem[BLK_DT];
    uint32_t next;
};
#define BLK_SZ sizeof(block_t)

inode_t* vfs_lookup(inode_t* ind, const char* name, int flags);
void init_blkfs(filesystem_t* fs);

#endif