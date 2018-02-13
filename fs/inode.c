/* This file is part of The Firekylin Operating System.
 *
 * Copyright (c) 2016, Liuxiaofeng
 * All rights reserved.
 *
 * This program is free software; you can distribute it and/or modify
 * it under the terms of The BSD License, see LICENSE.
 */

#include <sys/param.h>
#include <fs.h>
#include <sched.h>
#include <assert.h>
//sleeplock_t             inode_table_lock;

//#define lock_inode_table()	require_lock(&inode_table_lock);
//#define unlock_inode_table()	release_lock(&inode_table_lock);
#define lock_inode_table()
#define unlock_inode_table()
struct inode *iget(dev_t dev, ino_t ino) {
	extern struct super_block yaffs_sb;
	return iget_locked(yaffs_sb, ino);
	return NULL;
}

void iput(struct inode *inode) {
	assert(inode && inode->i_op && inode->i_op->evict_inode);
	if ((!inode) || (!inode->i_op) || (!inode->i_op->evict_inode))
		return;
	inode->i_count--;
	assert(inode->i_count >= 0);
	//TODO:暂时不操作
	//if (inode->i_count == 0)
	//	inode->i_op->evict_inode(inode);
}

extern dev_t mount_inode(struct inode *inode);

struct inode *namei(char *filepath, char **basename) {
	struct inode *inode;
	char name[MAX_NAME_LEN + 1];

	if (*filepath == '/') {
		filepath++;
		inode = idup(root_inode);
	} else
		inode = idup(CURRENT_TASK()->pwd);

	while (*filepath) {
		if (basename)
			*basename = filepath;

		for (int i = 0; i < MAX_NAME_LEN; i++) {
			if (*filepath == 0 || *filepath == '/') {
				name[i] = 0;
				break;
			}
			name[i] = *filepath++;
		}
		if (!*filepath && basename)
			return inode;
		if (*filepath == '/')
			filepath++;
		if (inode->i_op->lookup(inode, name, &inode)) {//找到返回0
			return NULL;
		}
	}
	return inode;
}
void sync_inode() {
	printf("sync_inode\n");
	// struct inode *inode = inode_table;

	// while (inode < inode_table + NR_INODE) {
	//ilock(inode);
	// if (inode->i_flags & I_DIRTY)
	// inode->i_op->inode_write(inode);
	//iunlock(inode);
	// inode++;
	// }
}
