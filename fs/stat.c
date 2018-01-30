#include <sys/stat.h>
#include <errno.h>
#include <kernel.h>
#include <fs.h>
#include <sched.h>
#include <syscall.h>
static void copy_stat(struct inode *inode , struct stat *statbuf) {
	statbuf->st_dev = inode->i_dev;
	statbuf->st_ino = inode->i_ino;
	statbuf->st_mode = inode->i_mode;
	statbuf->st_nlink = inode->i_nlink;
	statbuf->st_uid = inode->i_uid;
	statbuf->st_gid = inode->i_gid;
	statbuf->st_rdev = inode->i_rdev;
	statbuf->st_size = inode->i_size;
	statbuf->st_atime = inode->i_atime;
	statbuf->st_mtime = inode->i_mtime;
	statbuf->st_ctime = inode->i_ctime;
}

SYSCALL_DEFINE2(stat, char *, filename, struct stat *, statbuf) {
	struct inode *inode;

	if (!(inode = namei(filename, NULL)))
		return -ENOENT;

	copy_stat(inode, statbuf);
	iput(inode);
	return 0;
}

SYSCALL_DEFINE2(fstat, int , fd, struct stat *, statbuf) {
	struct inode *inode;

	if (fd > NR_OPEN || !(CURRENT_TASK()->files[fd]))
		return -EBADF;

	inode = idup(CURRENT_TASK()->files[fd]->f_inode);
	copy_stat(inode, statbuf);
	iput(inode);
	return 0;
}

SYSCALL_DEFINE3(chown, char *, filename, uid_t , uid, gid_t , gid) {
	struct inode *inode;

	if (CURRENT_TASK()->uid != 0)
		return -EPERM;
	if (!(inode = namei(filename, NULL)))
		return -ENOENT;

	inode->i_uid = uid;
	inode->i_gid = gid;
	inode->i_flags |= I_DIRTY;
	iput(inode);
	return 0;
}

SYSCALL_DEFINE2(chmod, char *, filename, mode_t , mode) {
	struct inode *inode;

	if (!(inode = namei(filename, NULL)))
		return -ENOENT;
	if (CURRENT_TASK()->uid != inode->i_uid && CURRENT_TASK()->uid != 0) {
		iput(inode);
		return -EACCES;
	}

	inode->i_mode = (inode->i_mode & ~07777) | (mode & 07777);
	inode->i_flags |= I_DIRTY;
	iput(inode);
	return 0;
}

SYSCALL_DEFINE2(utime, char *, filename, struct utimebuf *, timebuf) {
	struct inode *inode;

	if (!(inode = namei(filename, NULL)))
		return -ENOENT;

	if (timebuf) {
		inode->i_ctime = timebuf->ctime;
	}
	//inode->i_atime = inode->i_mtime = current_time();
	inode->i_atime = inode->i_mtime = 0;
	inode->i_flags |= I_DIRTY;
	iput(inode);
	return 0;
}
