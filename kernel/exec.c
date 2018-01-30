/*
exec.c:
Copyright (C) 2009  david leels <davidontech@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see http://www.gnu.org/licenses/.
*/
#include "elf.h"
#include "yaffsfs.h"
int exec(const char *filename){

	// struct elf32_phdr *phdr;
	// struct elf32_ehdr *ehdr;
	// int pos,dpos;
	// char *buf=NULL;


	// if(!(filename))
		// return 1;
	
	// int handle =yaffs_open(filename, O_RDONLY, S_IREAD|S_IWRITE);
	// if (handle ==-1) {
		// printf("Open %s failed\n",filename);
		// return 1;
	// }else{
		// printf("Open %s succeed\n",filename);
	// }
	// int filelen = yaffs_lseek(handle, 0, SEEK_END);
	// printf("file size:%d\n",filelen);
	// buf=kmalloc(filelen);
	// if(buf==NULL){
		// printf("kmalloc failed");
		// return 1;
	// }
	
	// int BytesRead= yaffs_read(handle, buf, filelen);
	// if(BytesRead == -1) {
		// printf("Read test.txt error\n");
		// yaffs_close(handle);
		// return 1;
	// }
	// printf("file read size:%d\n",BytesRead);
	// yaffs_close(handle);

	// ehdr=(struct elf32_ehdr *)buf;
	// phdr=(struct elf32_phdr *)((char *)buf+ehdr->e_phoff);


	// asm volatile(
		// "mov pc,r0\n\t"
	// );
	
	return 0;

}
