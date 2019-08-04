#ifndef __DEVFS_H__
#define __DEVFS_H__

#include<common.h>
#include<klib.h>
#include<am.h>
#include<devices.h>

inode_t* vfs_lookup(inode_t* ind, const char* name, int flags);
void init_devfs(filesystem_t* fs);

#endif