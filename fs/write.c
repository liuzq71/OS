#include <syscall.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <fs.h>
#include <sched.h>
SYSCALL_DEFINE3(write, int , fd, char *, buf, size_t , count) {
	struct file *file;
	int res = -EBADF;
	if (fd == 1) {
		if (!buf)
			return -EFAULT;
		printf(buf);
		return strlen(buf) + 1;
	}
	if (!buf)
		return -EFAULT;
	if (fd > NR_OPEN || !(file = CURRENT_TASK()->files[fd]))
		return -EBADF;
	if (!((file->f_mode & O_ACCMODE) == O_WRONLY || (file->f_mode & O_ACCMODE) == O_RDWR))
		return -EBADF;
	if ((!file->f_inode) || (!file->f_inode->i_op) || (!file->f_inode->i_op->file_write))
		return -EINVAL;
	if (!count)
		return 0;

	//lock_file(file);
	//lock_inode(file->f_inode);

	if (file->f_mode & O_APPEND)
		file->f_pos = file->f_inode->i_size;
	switch (file->f_inode->i_mode & S_IFMT) {
		case S_IFREG:
			res = file->f_inode->i_op->file_write(file, buf, count , &(file->f_pos));
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

	if ((S_ISREG(file->f_inode->i_mode) || S_ISDIR(file->f_inode->i_mode))
	        && file->f_pos > file->f_inode->i_size) {
		file->f_inode->i_size = file->f_pos;
		//file->f_inode->i_ctime = current_time();
		file->f_inode->i_flags |= I_DIRTY;
	}

	//unlock_inode(file->f_inode);
	//unlock_file(file);

	return res;
}
