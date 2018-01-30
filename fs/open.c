#include <stdio.h>
#include <syscall.h>
#include <sys/err.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/unistd.h>
#include <fs.h>
#include <sched.h>
int get_unused_fd(void) {
	int fd;
	for (fd = 3; fd < NR_OPEN; fd++) {
		if (!CURRENT_TASK()->files[fd]) {
			return fd;
		}
	}
	if (fd >= NR_OPEN)
		return -EINVAL;
}

static struct file *do_filp_open(int dfd, const char *filename, int flags,
                                 int mode) {

	int namei_flags, error;
	// /*创建nameidata结构体，返回的安装点对象和目录项对象放在此结构体*/
	// struct nameidata nd;
	// namei_flags = flags;
	// if ((namei_flags+1) & O_ACCMODE)
	// namei_flags++;
	// /*根据上级的dentry对象得到新的dentry结构，并从中得到相关的inode节点号，再用iget函数分配新的inode结构，将新的dentry对象与inode对象关联起来*/
	// error = open_namei(dfd, filename, namei_flags, mode, &nd);
	// /*将nameidata结构体转化为struct file文件对象结构体*/
	// if (!error)
	// return nameidata_to_filp(&nd, flags);

	return ERR_PTR(error);
}
//将进程的current->files对象与file文件对象进行绑定，从而直接操作定义的方法
void fd_install(unsigned int fd, struct file *file) {
	/*进程的files_struct对象*/
	CURRENT_TASK()->files[fd] = file;
}
/*对struct file结构体赋值*/
// static struct file *__dentry_open(struct dentry *dentry, struct vfsmount *mnt,
// int flags, struct file *f,
// int (*open)(struct inode *, struct file *))
// {

// file_move(f, &inode->i_sb->s_files);

// return f;
// }
SYSCALL_DEFINE3(open, const char *, path, int, flags, int, mode) {
	int fd = get_unused_fd();
	if (IS_ERR(fd))
		return fd;
	struct file *file;
	/*file对象是文件对象,存在于内存，所以没有回写，f_op被赋值*/
	// struct file *f = do_filp_open(dfd, tmp, flags, mode);
	// if (IS_ERR(f)) {
	// put_unused_fd(fd);
	// fd = PTR_ERR(f);
	// } else {
	// fsnotify_open(f->f_path.dentry);
	// /*将current->files_struct和文件对象关联*/
	// fd_install(fd, f);
	// }
	struct inode *inode;
	printf("sys_open start!\n");
	if (!(inode = namei(path, NULL))) {
		if (!(flags & O_CREAT)) {
			return -ENOENT;
		}
		if (sys_mknod(path, S_IFREG | (mode & 07777), 0) < 0)
			return -EAGAIN;
		if (!(inode = namei(path, NULL))) {
			printf("open error!\n");
			return -EAGAIN;
		}

	} else if (flags & O_EXCL) { //TODO
		return -EEXIST;
	}
	file = kmalloc(sizeof(struct file));
	//iunlock(inode);
	struct dentry *dentry = kmalloc(sizeof(struct dentry));

	file->f_count = 1;
	file->f_inode = inode;
	file->f_dentry = dentry;
	file->f_dentry->d_inode = inode;
	file->f_pos = 0;
	file->f_flags = flags;
	file->f_mode = flags;
	fd_install(fd, file);
	printf("sys_open end! fd = %d\n", fd);
	return fd;
}
SYSCALL_DEFINE1(chdir, char *, path) {
	struct inode *inode;
	if (!(inode = namei(path, NULL)))
		return -ENOENT;
	if (!(S_ISDIR(inode->i_mode))) {
		iput(inode);
		return -ENOTDIR;
	}

	iput(CURRENT_TASK()->pwd);
	CURRENT_TASK()->pwd = inode;
	//iunlock(inode);
	return 0;
}
SYSCALL_DEFINE2(access, char *, filename, int , mode) {
	struct inode *inode;
	mode_t tmp_mode = 0;

	if (!(inode = namei(filename, NULL)))
		return -EACCES;
	if (mode == F_OK) {
		iput(inode);
		return 0;
	}

	mode &= 7;
	//if (current->uid == inode->i_uid)
	tmp_mode |= (mode) << 6;
	//if (current->gid == inode->i_gid)
	tmp_mode |= (mode) << 3;
	tmp_mode |= (mode);
	//TODO:
	if ((tmp_mode & inode->i_mode)) {
		iput(inode);
		return 0;
	}

	iput(inode);
	return -EACCES;
}
