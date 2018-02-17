#include <sys/types.h>
#include <assert.h>

#include <syscall.h>
//TODO
#include <sys/syscall.h>
#include <sys/err.h>
#include <sys/dirent.h>
#include <sys/stat.h>
#include <sched.h>
#include <rtc.h>
#include "yaffsfs.h"
#include "command.h"
#define CMD_MAX_CMD_NUM 50
#define CMD_MAXARGS 10
#undef kmalloc
#undef kfree
#include <sys/mm.h>
static const char *yaffs_file_type_str(struct yaffs_stat *stat) {
	switch (stat->st_mode & S_IFMT) {
		case S_IFREG:
			return "regular file";
		case S_IFDIR:
			return "directory";
		case S_IFLNK:
			return "symlink";
		default:
			return "unknown";
	}
}

int yaffs_ls(const char *mountpt, int longlist) {
	int i;
	yaffs_DIR *d;
	struct yaffs_dirent *de;
	struct yaffs_stat stat;
	char tempstr[255 + 1];

	d = yaffs_opendir(mountpt);

	if (!d) {
		printf("opendirfailed, %s\n", yaffs_error_to_str(yaffsfs_GetLastError()));
		return -1;
	}

	for (i = 0; (de = yaffs_readdir(d)) != NULL; i++) {
		if (longlist) {
			sprintf(tempstr, "%s/%s", mountpt, de->d_name);
			yaffs_lstat(tempstr, &stat);
			printf("%-25s\t%7ld",
			       de->d_name,
			       (long)stat.st_size);
			printf("%5d %s\n",
			       stat.st_ino,
			       yaffs_file_type_str(&stat));
		} else {
			printf("%s\n", de->d_name);
		}
	}

	yaffs_closedir(d);

	return 0;
}

extern cmd_table *ct_list[];

CMD_DEFINE(ls, "ls [OPTION] [FILE]", "NULL") {
	int flag = 0;
	char *path = NULL;
	if (argc < 2)
		return 1;
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] != '-') {
			path = argv[i];
		} else {
			if (argv[i][1] == 'l')
				flag = 1;
		}
	}
	if (!path)
		return 1;
	yaffs_ls(path, flag);
	return 0;
}
CMD_DEFINE(ls_test, "ls [OPTION] [FILE]", "NULL") {
	int ret;
	if (argc != 2)
		return 1;
	int fd = syscall(__NR_open, argv[1], O_RDONLY);
	if (IS_ERR(fd)) {
		printf("ls_test open fd = %d\n", fd);
		return 1;
	}
	char buf[sizeof(struct dirent) + 256] = {0};
	printf("ls_test buf = 0x%02X\n", buf);
	do {
		ret = syscall(__NR_getdents, fd, buf, sizeof(struct dirent) + 256);
		if (IS_ERR(ret))
			return 1;
		struct dirent *dirent = buf;
		if (dirent->d_off == 0)
			break;
		printf("%25s\n", dirent->d_name);
		do {
			dirent = (char *)dirent + dirent->d_reclen;
			printf("%25s\n", dirent->d_name);
		} while (dirent->d_off);
	} while (ret);
	return 0;
}
CMD_DEFINE(rm, "NULL", "NULL") {
	char *path = NULL;
	if (argc == 2 && argv[1][0]) {
		printf("rm %s\n", argv[1]);
		if (!syscall(__NR_unlink, argv[1])) {
			printf("rm succeed!\n");
		} else {
			printf("rm failed!\n");
		}
	} else {
		printf("argument error!\n");
	}

	return 0;
}
CMD_DEFINE(cd, "NULL", "NULL") {
	if (argc == 2)
		syscall(__NR_chdir, argv[1]);
	return 0;
}
CMD_DEFINE(touch, "NULL", "NULL") {
	char *filename = NULL;
	char *test = NULL;
	int ret = 0;
	if (argc < 2)
		return 1;
	filename = argv[1];
	if (argc == 3)
		test = argv[2];
	if (!(filename))
		return 1;
	int handle = syscall(__NR_open, filename, O_CREAT | O_WRONLY | O_APPEND, S_IREAD | S_IWRITE);
	printf("open ret = %d\n", handle);
	if (handle == -1) {
		printf("Create %s failed\n", filename);
		return 1;
	} else {
		printf("Create %s succeed\n", filename);
	}
	if (test != NULL) {
		ret = syscall(__NR_write, handle, test, strlen(test) + 1);
		printf("write ret = %d\n", ret);
	}

	ret = syscall(__NR_close, handle);
	printf("close ret = %d\n", ret);
	return 0;
}
CMD_DEFINE(cat, "NULL", "NULL") {
	char Text[256] = {0};
	int BytesRead;
	char *filename = NULL;
	if (argc != 2)
		return 1;
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] != '-') {
			filename = argv[i];
		}
	}
	if (!(filename))
		return 1;

	int handle = syscall(__NR_open, filename, O_RDONLY);
	if (IS_ERR(handle)) {
		printf("Open %s failed fd = %d\n", filename, handle);
		return 1;
	} else {
		printf("Open %s succeed fd = %d\n", filename, handle);
	}
	while (1) {
		BytesRead = syscall(__NR_read, handle, Text, sizeof(Text) - 1);
		if (IS_ERR(BytesRead)) {
			printf("Read %s error! ret = %d\n", argv[1], BytesRead);
			syscall(__NR_close, handle);
			break;
		}
		Text[BytesRead] = 0;
		printf("%s", Text);
		if (BytesRead < (sizeof(Text) - 1)) {
			syscall(__NR_close, handle);
			break;
		}
	}
	return 0;
}
CMD_DEFINE(erase, "erase nand flash blocks", "help") {
	printf("cmd name:%s\n", ct->name);
	for (int i = 0; i < argc; i++) {
		printf("argv[%d]:%s\n", i, argv[i]);
	}
	for (int i = 19; i < 2048; i++) {
		if (nand_erase_block(i) != 1)
			printf("erase failed! block number:%d addr:%X\n", i, i << 17);
	}
	printf("erase finish!\n");
	return 0;
}
CMD_DEFINE(syscall, "syscall", "syscall") {
	printf("cmd name:%s\n", ct->name);
	char *str = argv[2];
	printf("%s\n", str);
	printf("arg0=%0X\n", str);
	printf("add of arg0=%0X\n", &str);
	int syscall_num = argv[1][0] - '0';
	__asm__ volatile (
	    "mov r0,%0\n"
	    "mov r7,%1\n"
	    "swi #0\n"
	    :
	    :"r"(str), "r"(syscall_num)
	    :"r0", "r7"
	);
	return 0;
}
CMD_DEFINE(usbslave,
           "usbslave - get file from host(PC)\n",
           "[loadAddress] [wait] \n"
           "\"wait\" is 0 or 1, 0 means for return immediately, not waits for the finish of transferring\n") {
	//TODO:最好将文件下载到文件系统中
	extern int download_run;
	extern volatile U32 dwUSBBufBase;
	extern volatile U32 dwUSBBufSize;

	int g_bUSBWait = 1;
#define BUF_SIZE (1024*1024)
	/* download_run为1时表示将文件保存在USB Host发送工具dnw指定的位置
	 * download_run为0时表示将文件保存在参数argv[2]指定的位置
	 * 要下载程序到内存，然后直接运行时，要设置download_run=1，这也是这个参数名字的来由
	 */
	//由于0x3000000存放了页表，必须download_run = 0确保下载地址正确，即不采用上位机设置的地址
	download_run = 0;//默认由下位机决定地址和大小
	if (argc == 3) {
		dwUSBBufBase = kmalloc(BUF_SIZE);
		if(!dwUSBBufBase){
			printf("malloc memory error!\n");
			return 1;
		}
		g_bUSBWait = (int)simple_strtoul(argv[1], NULL, 16);
		dwUSBBufSize = BUF_SIZE;
	}else{
		return 1;
	}
	int size = usb_receive(dwUSBBufBase, dwUSBBufSize, g_bUSBWait);
	int ret = 0;
	if(size > 0 && size <= BUF_SIZE){
		int handle = syscall(__NR_open, argv[2], O_CREAT | O_WRONLY , S_IREAD | S_IWRITE);
		printf("open ret = %d\n", handle);
		if (handle == -1) {
			printf("Create %s failed\n", argv[2]);
			return 1;
		}
		ret = syscall(__NR_write, handle, dwUSBBufBase, size);
		printf("write ret = %d\n", ret);
		ret = syscall(__NR_close, handle);
		printf("close ret = %d\n", ret);
	}
	return 0;
}
CMD_DEFINE(mplay, "mplay", "mplay") {
	mplay();
	return 0;
}
CMD_DEFINE(task_test, "task_test", "task_test") {
	task_init();
	return 0;
}
CMD_DEFINE(assert_test, "assert_test", "assert_test") {
	assert(1);
	assert(1 == 1);
	assert(1 != 1);
	return 0;
}
CMD_DEFINE(malloc_test, "kmalloc_test", "kmalloc_test") {
	int size;
	void *addr;
#define KMALLOC_TEST(x)\
		addr = kmalloc(size = x);\
		printf("kmalloc:size = %d, addr = %X\n", size, addr);\
		kfree(addr);printf("end\n\n");
	KMALLOC_TEST(0);
	KMALLOC_TEST(1);
	KMALLOC_TEST(2);
	KMALLOC_TEST(31);
	KMALLOC_TEST(32);
	KMALLOC_TEST(4095);
	KMALLOC_TEST(4096);
	KMALLOC_TEST(PAGE_8K);
	KMALLOC_TEST(PAGE_8K);
	//KMALLOC_TEST(PAGE_1M);
	// KMALLOC_TEST(PAGE_1M);
#define PAGE_MALLOC_TEST(x,y)\
		addr = get_free_pages(y, size = x);\
		printf("get_free_pages:size = %d, addr = %X\n", 1<<size, addr);\
		put_free_pages(addr,x);printf("\n");
	PAGE_MALLOC_TEST(PAGE_ORDER_4K,PAGE_ALIGN_8K);
	PAGE_MALLOC_TEST(PAGE_ORDER_4K,PAGE_ALIGN_128K);
	PAGE_MALLOC_TEST(PAGE_ORDER_8K,PAGE_ALIGN_8K);
	PAGE_MALLOC_TEST(PAGE_ORDER_8K,PAGE_ALIGN_16K);
	PAGE_MALLOC_TEST(PAGE_ORDER_1M,PAGE_ALIGN_16K);
	return 0;
}
CMD_DEFINE(delay, "delay", "delay") {
	if (argv < 2)return;
	for (volatile int i = 0; i < 10; i++) {
		printf("delay!");
		delay_u(simple_strtoul(argv[1], NULL, 10));
	}
	return 0;
}
CMD_DEFINE(delay_l, "delay", "delay") {
	if (argv < 2)return;
	int time = simple_strtoul(argv[1], NULL, 10);
	for (volatile int i = 0; i < 10; i++) {
		for (volatile int i = 0; i < 10000; i++)
			for (volatile int i = 0; i < time; i++);
		printf("delay!");
	}
	return 0;
}
CMD_DEFINE(fs, "fs", "fs") {
	fs_test();
	return 0;
}
CMD_DEFINE(color, "color", "color") {
	//TODO:
	printf("\033\x40\x311231414");
	return 0;
}
CMD_DEFINE(rename, "rename", "rename") {
	if (argc == 3) {
		char *old = argv[1];
		char *new = argv[2];
		int ret = syscall(__NR_rename, old, new);
		if (IS_ERR(ret)) {
			printf("rename failed!\n");
			return 1;
		} else {
			printf("rename succeed!\n");
		}

		return 0;
	}
	return 1;
}
CMD_DEFINE(mkdir, "mkdir", "mkdir") {
	if (argc == 2) {
		int ret = syscall(__NR_mkdir, argv[1]);
		if (IS_ERR(ret)) {
			printf("rename failed!\n");
			return 1;
		} else {
			printf("rename succeed!\n");
		}
		return 0;
	}
	return 1;
}
CMD_DEFINE(RTC, "RTC", "RTC") {
	char data[7] = {0};
	char *week_str[7] = {"一", "二", "三", "四", "五", "六", "日"};
	char *week;
	if (argc == 1) {
		RTC_Read(&data[0], &data[1], &data[2], &data[3], &data[4], &data[5], &data[6]);

		if (data[3] >= 1 && data[3] <= 7) {
			week = week_str[data[3] - 1];
			printf("%d年,%d月,%d日,星期%s,%d点,%d分,%d秒\n", 2000 + data[0],
			       data[1], data[2], week, data[4], data[5], data[6]);
		}else{
			printf("error!\n");
			return 1;
		}
	} else if (argc == 8) {
		for (int i = 0; i < 7; i++) {
			data[i] = simple_strtoul(argv[i + 1], NULL, 10);
		}
		//year:0-99
		RTC_Set(data[0], data[1], data[2], data[3], data[4], data[5], data[6]);
		if (data[3] >= 1 && data[3] <= 7) {
			week = week_str[data[3] - 1];
			printf("%d年,%d月,%d日,星期%s,%d点,%d分,%d秒\n", 2000 + data[0],
					   data[1], data[2], week, data[4], data[5], data[6]);
			printf("设置成功\n");
		}else{
			printf("error!\n");
			return 1;
		}
	}else{
		printf("error!参数数量异常\n");
		return 1;
	}
	return 0;
}
CMD_DEFINE(help, "help", "help") {
	printf("cmd name:%s\n", ct->name);
	for (int i = 0; i < argc; i++) {
		printf("argv[%d]:%s\n", i, argv[i]);
	}
	for (int i = 0; ct_list[i] != NULL; i++) {
		printf("%s:\t-%s\n", ct_list[i]->name, ct_list[i]->usage);
	}
	return 0;
}
cmd_table *ct_list[] = {
	&ct_help,
	&ct_ls,
	&ct_ls_test,
	&ct_cat,
	&ct_cd,
	&ct_touch,
	&ct_erase,
	&ct_rm,
	&ct_syscall,
	&ct_usbslave,
	&ct_mplay,
	&ct_task_test,
	&ct_assert_test,
	&ct_malloc_test,
	&ct_fs,
	&ct_delay,
	&ct_delay_l,
	&ct_color,
	&ct_rename,
	&ct_mkdir,
	&ct_RTC,
	NULL
};

cmd_table *search_cmd(char *name) {
	for (int i = 0; ct_list[i] != NULL; i++) {
		if (strcmp(ct_list[i]->name, name) == 0) {
			return ct_list[i];
		}
	}
	return NULL;
}
int run_command (char *cmd, int flag) {
	char *str = cmd;
	char *argv[CMD_MAXARGS + 1] = {0};	/* NULL terminated	*/
	int argc = 0;
	int cmdlen = strlen(cmd);
	for (int i = 0; i < cmdlen; i++) {
		if (str[i] != ' ' && i != 0) {
			continue;
		} else {
			while (str[i] == ' ') {
				str[i] = '\0';
				i++;
			}
			if (i < cmdlen) {
				argv[argc] = &str[i];
				argc++;
				if (argc == CMD_MAXARGS + 1)
					return -1;
			} else
				break;
		}
	}
	cmd_table *pct = search_cmd(argv[0]);
	if (pct) {
		pct->cmd(pct, argc, argv);
	} else {
		printf("%s:command not found\n", argv[0]);
		return 0;
	}
	return 1;
}
static int get_str(char *buf, int len) {
	int i;
	for (i = 0; i < len - 1; i++) {
		char c = getc();
		//xshell 回车产生\r\n
		if (c == '\r') {
			getc();

			if (i == 0) {
				return -1;
			} else {
				printf("\n");
				buf[i] = '\0';
				break;
			}
		} else if (c == '\b') {
			if (i > 0) { //前面有字符
				putc(c);
				i = i - 2;
			} else { //前面没有字符
				i = i - 1;
			}
		} else {
			putc(c);
			buf[i] = c;
		}
	}
	return 1;
}
int cmd_loop() {
	char buf[100] = {0};
	while (1) {
		printf("\nOS>");
		if (get_str(buf, 100) == -1)
			continue;
		run_command (buf, 1);
	}
}
