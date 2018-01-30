#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <kernel.h>
#include <syscall.h>
#include <sched.h>
#include <fs.h>
#include <kernel.h>
// static int read_char(struct file *file, char *buf, size_t size)
// {
// dev_t dev = file->f_inode->i_rdev;
// int major = MAJOR(dev);
// if (major > DEV_CHAR_MAX || !char_table[major])
// panic("dev %x not exsit", dev);
// return char_table[major]->read(dev, buf, file->f_pos, size);
// }

// static int read_blk(struct file *file, char *buf, size_t size)
// {
// struct buffer *bh;
// int chars, left, off;

// left = size;
// off = file->f_pos;

// while (left) {
// bh = bread(file->f_inode->i_rdev, off / 1024);
// if (!bh)
// return -EIO;
// chars = min(left, 1024 - off % 1024);
// memcpy((char*) buf, (char*) (bh->b_data + off % 1024), chars);
// brelse(bh);
// buf += chars;
// off += chars;
// left -= chars;
// }
// return size - left;
// }

// extern int read_pipe(struct file *file, char *buf, size_t size);
SYSCALL_DEFINE3(read, int, fd, char *, buf, size_t, count) {
	struct file *file;
	int res = -1;
	printf("sys_read fd:%d,buf=%X,count=%d\n", fd, buf, count);
	if (fd > NR_OPEN || !(file = CURRENT_TASK()->files[fd])) {
		printf("EBADF1\n");
		return -EBADF;
	}
	if (!((file->f_mode & O_ACCMODE) == O_RDONLY || (file->f_mode & O_ACCMODE) == O_RDWR)) {
		printf("EBADF2\n");
		return -EBADF;
	}
	if ((!file->f_inode) || (!file->f_inode->i_op) || (!file->f_inode->i_op->file_read))
		return -EINVAL;
	if (!count)
		return 0;
	if (!buf)
		return -EFAULT;
	//lock_file(file);
	//lock_inode(file->f_inode);

	switch (file->f_inode->i_mode & S_IFMT) {
		case S_IFREG:
			printf("sys_read ,f_inode:%X\n", file->f_inode);
			res = file->f_inode->i_op->file_read(file, buf, count , &(file->f_pos));
			printf("sys_read res = %d\n", res);
			break;
		case S_IFDIR:
			res = -EISDIR;
			//res = file->f_inode->i_op->file_readdir(file, buf, count);
			break;
		case S_IFCHR:
			//res = read_char(file, buf, size);
			break;
		case S_IFBLK:
			//res = read_blk(file, buf, size);
			break;
		case S_IFIFO:
			//res = read_pipe(file, buf, size);
			break;
		default:
			printf("EIO\n");
			res = -EIO;
	}

	// if (res > 0)
	// file->f_pos += res;

	//unlock_inode(file->f_inode);
	//unlock_file(file);

	return res;
}
struct getdents_callback {
	struct dirent   *current_dir;
	struct dirent   *previous;
	int count;
	int error;
};
static int filldir(void *buf, const char *name, int namlen, loff_t offset, u32 ino, unsigned int d_type) {
	//由于ARM架构，一定要4字节对齐
	int reclen = ALIGN(sizeof(struct dirent) + namlen + 1, sizeof(int));////TODO:可能多加一点，万一宽字符
	struct getdents_callback *gc = buf;
	struct dirent *dirent;
	printf("filldir start!\n");
	if (reclen > gc->count) {
		dirent = gc->previous;
		if (dirent) {
			dirent->d_off = 0;
		}
		return -EINVAL;
	}

	dirent = gc->previous;
	if (dirent) {
		dirent->d_off = offset;
	}
	dirent = gc->current_dir;
	dirent->d_ino = ino;
	dirent->d_reclen = reclen;
	dirent->d_type = d_type;
	if (namlen > 0) {
		strcpy(dirent->d_name, name);//TODO
	}


	gc->previous = gc->current_dir;
	gc->current_dir = (char *)gc->current_dir + reclen;
	gc->count -= reclen;
	printf("filldir end!\n");
	return 0;
}

SYSCALL_DEFINE3(getdents, unsigned int, fd, void *, buf, unsigned int, count) {
	struct file *file;
	int res = -1;
	if (fd > NR_OPEN || !(file = (CURRENT_TASK())->files[fd])) {
		return -EBADF;
	}
	if (!((file->f_mode & O_ACCMODE) == O_RDONLY || (file->f_mode & O_ACCMODE) == O_RDWR)) {
		return -EBADF;
	}
	if ((!file->f_inode) || (!file->f_inode->i_op) || (!file->f_inode->i_op->file_readdir))
		return -EINVAL;
	if (!count)
		return 0;
	if (!buf)
		return -EFAULT;

	if ((file->f_inode->i_mode & S_IFMT) != S_IFDIR) {
		res = -ENOTDIR;
	} else {
		struct getdents_callback gc;
		gc.current_dir = buf;
		gc.previous = NULL;
		gc.count = count;
		res = file->f_inode->i_op->file_readdir(file, &gc, filldir);
	}
	return res;
}

// long exec_load_file(struct file *file, char *buf)
// {
// Elf32_Ehdr *ehdr;
// Elf32_Phdr *phdr;

// struct task *current = CURRENT_TASK();

// ehdr = (Elf32_Ehdr *) buf;

// for (int i = 0; i < ehdr->e_phnum; i++) {
// phdr = (Elf32_Phdr *) (buf + ehdr->e_phoff
// + i * ehdr->e_phentsize);
// if (!phdr->p_vaddr)
// continue;
// alloc_mm(current->pdtr, phdr->p_vaddr, phdr->p_memsz);
// file->f_pos = phdr->p_offset;
// file->f_inode->i_op->file_read(file, (char*) phdr->p_vaddr,
// phdr->p_filesz);
// current->sbrk = phdr->p_vaddr + phdr->p_memsz;
// phdr++;
// }
// current->sbrk = (current->sbrk + 0xf) & 0xfffffff0;

// return ehdr->e_entry;
// }
