/* This file is part of The Firekylin Operating System.
 *
 * Copyright (c) 2016, Liuxiaofeng
 * All rights reserved.
 *
 * This program is free software; you can distribute it and/or modify
 * it under the terms of The BSD License, see LICENSE.
 */

#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <syscall.h>
#include <fs.h>

SYSCALL_DEFINE3(mknod, char *, filename, mode_t , mode, dev_t , dev) {
	struct inode *dir_inode;
	char *basename;
	int res;
	printf("sys_mknod start!\n");
	if (S_ISDIR(mode) || S_ISLNK(mode))
		return -EINVAL;
	if (!(dir_inode = namei(filename, &basename)))
		return -ENOENT;
	if (!*basename) {
		iput(dir_inode);
		return -ENOENT;
	}
	res = dir_inode->i_op->mknod(dir_inode, basename, mode, dev);
	iput(dir_inode);
	return res;
}

SYSCALL_DEFINE2(mkdir, char *, pathname, mode_t , mode) {
	struct inode *dir_inode;
	char *basename;
	int res;

	if (!(dir_inode = namei(pathname, &basename)))
		return -ENOENT;
	if (!*basename) {
		iput(dir_inode);
		return -EINVAL;
	}

	mode = S_IFDIR | (mode & 07777);
	res = dir_inode->i_op->mkdir(dir_inode, basename, mode);
	iput(dir_inode);
	return res;
}

int sys_link(char *name, char *newname) {
	struct inode *inode, *new_dir_inode;
	char *basename;
	int res;

	if (!(inode = namei(name, NULL)))
		return -ENOENT;
	if (S_ISDIR(inode->i_mode)) {
		iput(inode);
		return -EPERM;
	}
	if (!(new_dir_inode = namei(newname, &basename))) {
		iput(inode);
		return -EACCES;
	}
	if (!*basename) {
		iput(inode);
		iput(new_dir_inode);
		return -EACCES;
	}
	if (inode->i_dev != new_dir_inode->i_dev) {
		iput(inode);
		iput(new_dir_inode);
		return -EACCES;
	}
	res = new_dir_inode->i_op->link(new_dir_inode, basename, inode);
	iput(inode);
	iput(new_dir_inode);
	return res;
}

SYSCALL_DEFINE1(unlink, char *, pathname) {
	struct inode *dir_inode;
	char *basename;
	int res;

	if (!(dir_inode = namei(pathname, &basename)))
		return -ENOENT;
	if (!*basename) {
		iput(dir_inode);
		return -EACCES;
	}

	res = dir_inode->i_op->unlink(dir_inode, basename);
	iput(dir_inode);
	return res;
}

SYSCALL_DEFINE1(rmdir, char *, pathname) {
	struct inode *dir_inode;
	char *basename;

	if (!(dir_inode = namei(pathname, &basename)))
		return -ENOENT;
	if (!*basename) {
		iput(dir_inode);
		return -ENOENT;
	}

	return dir_inode->i_op->rmdir(dir_inode, basename);
}

SYSCALL_DEFINE2(rename, char *, old, char *, new) {
	struct inode *old_dir_inode, *new_dir_inode;
	char *old_basename, *new_basename;
	int res;

	old_dir_inode = namei(old, &old_basename);
	//iunlock(old_dir_inode);
	new_dir_inode = namei(new, &new_basename);

	if (!old_dir_inode) {
		//iput(iunlock(old_dir_inode));
		return -ENOENT;
	}
	if (old_dir_inode != new_dir_inode) {
		//iput(ilock(old_dir_inode));
		iput(new_dir_inode);
		return -1;
	}

	res = new_dir_inode->i_op->rename(old_dir_inode, old_basename, new_dir_inode,
	                                  new_basename);
	//iput(ilock(old_dir_inode));
	iput(old_dir_inode);
	iput(new_dir_inode);
	return res;
}
