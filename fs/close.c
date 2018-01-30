/* This file is part of The Firekylin Operating System.
 *
 * Copyright (c) 2016, Liuxiaofeng
 * All rights reserved.
 *
 * This program is free software; you can distribute it and/or modify
 * it under the terms of The BSD License, see LICENSE.
 */

#include <sys/stat.h>
#include <errno.h>
#include <kernel.h>
#include <sched.h>
#include <fs.h>
#include <syscall.h>
SYSCALL_DEFINE1(close, int, fd) {
	int res = -EBADF;
	struct file *file;
	struct inode *inode;
	if (fd > NR_OPEN || !(file = (CURRENT_TASK())->files[fd]))
		return -EBADF;
	if ((!file->f_inode) || (!file->f_inode->i_op) || (!file->f_inode->i_op->file_release))
		return -EINVAL;
	switch (file->f_inode->i_mode & S_IFMT) {
		case S_IFREG:
			res = file->f_inode->i_op->file_release(file);
			break;
		case S_IFDIR:
			res = -EISDIR;
			break;
		case S_IFCHR:
			//res = write_char(file, buf, size);
			break;
		case S_IFBLK:
			//res = write_blk(file, buf, size);
			break;
		case S_IFIFO:
			//res = write_pipe(file, buf, size);
			break;
		default:
			res = -EIO;
	}

	CURRENT_TASK()->files[fd] = NULL;
	inode = file->f_inode;
	if (--file->f_count < 0)
		printf("close:file count is 0\n");
	if (!file->f_count)
		iput(inode);
	if (file->f_dentry)
		kfree(file->f_dentry);
	kfree(file);
	return res;
}
