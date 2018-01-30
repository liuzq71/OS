#include "stdio.h"
//#include "stdlib.h"
//#include "time.h"

static int yaffs_errno;

/*
 * yaffs_bug_fn()
 * Function to report a bug.
 */
void yaffs_bug_fn(const char *fn, int n)
{
	printf("yaffs bug at %s:%d\n", fn, n);
}

/*
 * yaffsfs_CurrentTime() retrns a 32-bit timestamp.
 *
 * Can return 0 if your system does not care about time.
 */
unsigned int yaffsfs_CurrentTime(void)
{
	return 0;
}

/*
 * yaffsfs_SetError() and yaffsfs_GetLastError()
 * Do whatever to set the system error.
 * yaffsfs_GetLastError() just fetches the last error.
 */
void yaffsfs_SetError(int err)
{
	yaffs_errno = err;
}

int yaffsfs_GetLastError(void)
{
	return yaffs_errno;
}

/*
 * yaffsfs_CheckMemRegion()
 * Check that access to an address is valid.
 * This can check memory is in bounds and is writable etc.
 *
 * Returns 0 if ok, negative if not.
 */
int yaffsfs_CheckMemRegion(const void *addr, size_t size, int write_request)
{
	if(!addr) {
		return -1;
	}
	return 0;
}

/*
 * yaffsfs_malloc()
 * yaffsfs_free()
 *
 * Functions to allocate and free memory.
 */
void *yaffsfs_malloc(size_t size)
{
	return kmalloc(size);
}

void yaffsfs_free(void *ptr)
{
	kfree(ptr);
}

/*
 * yaffsfs_Lock()
 * yaffsfs_Unlock()
 * A single mechanism to lock and unlock yaffs. Hook up to a mutex or whatever.
 */
void yaffsfs_Lock(void)
{
	
}

void yaffsfs_Unlock(void)
{
	
}

void yaffsfs_OSInitialisation(void)
{
	/* No locking used */
}
