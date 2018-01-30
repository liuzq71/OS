#include <fs.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/err.h>
#define __YAFFSFS_H__//防止重定义
#include "yportenv.h"
#include "yaffs_trace.h"
#include "yaffs_guts.h"
#include "yaffs_attribs.h"
extern void *(kmalloc)(unsigned int size);
extern void (kfree)(void *addr);
struct inode *get_root_inode();
extern struct fs_operation yaffs_op;
struct super_block yaffs_sb = {0};
struct inode          *root_inode;

static int lookup(struct inode *inode, char *filename, struct inode **res);
//TODO:
#define CURRENT_TIME 0
#define yaffs_inode_to_obj_lv(iptr) ((iptr)->i_private)
#define yaffs_inode_to_obj(iptr)\
	((struct yaffs_obj *)(yaffs_inode_to_obj_lv(iptr)))
#define yaffs_dentry_to_obj(dptr) yaffs_inode_to_obj((dptr)->d_inode)
#define yaffs_super_to_dev(sb)	((struct yaffs_dev *)sb->s_fs_info)

#define update_dir_time(dir) do {\
			(dir)->i_ctime = (dir)->i_mtime = CURRENT_TIME; \
		} while (0)

static inline void inode_dec_link_count(struct inode *inode)
{
	if(inode->i_nlink>0)
		inode->i_nlink--;
}			
static void yaffs_gross_lock(struct yaffs_dev *dev)
{
	//yaffs_trace(YAFFS_TRACE_LOCK, "yaffs locking %p", current);
	//mutex_lock(&(yaffs_dev_to_lc(dev)->gross_lock));
	//yaffs_trace(YAFFS_TRACE_LOCK, "yaffs locked %p", current);
}

static void yaffs_gross_unlock(struct yaffs_dev *dev)
{
	//yaffs_trace(YAFFS_TRACE_LOCK, "yaffs unlocking %p", current);
	//mutex_unlock(&(yaffs_dev_to_lc(dev)->gross_lock));
}
static void yaffs_fill_inode_from_obj(struct inode *inode,
				      struct yaffs_obj *obj)
{
	u32 mode;

	if (!inode || !obj)  {
		yaffs_trace(YAFFS_TRACE_OS,
			"yaffs_fill_inode invalid parameters");
		return;
	}

	/* Check mode against the variant type
	 * and attempt to repair if broken. */
	mode = obj->yst_mode;

	switch (obj->variant_type) {
	case YAFFS_OBJECT_TYPE_FILE:
		if (!S_ISREG(mode)) {
			obj->yst_mode &= ~S_IFMT;
			obj->yst_mode |= S_IFREG;
		}
		break;
	case YAFFS_OBJECT_TYPE_SYMLINK:
		if (!S_ISLNK(mode)) {
			obj->yst_mode &= ~S_IFMT;
			obj->yst_mode |= S_IFLNK;
		}
		break;
	case YAFFS_OBJECT_TYPE_DIRECTORY:
		if (!S_ISDIR(mode)) {
			obj->yst_mode &= ~S_IFMT;
			obj->yst_mode |= S_IFDIR;
		}
		break;
	case YAFFS_OBJECT_TYPE_UNKNOWN:
	case YAFFS_OBJECT_TYPE_HARDLINK:
	case YAFFS_OBJECT_TYPE_SPECIAL:
	default:
		/* TODO? */
		break;
	}

	//inode->i_flags |= S_NOATIME;
	inode->i_ino = obj->obj_id;
	inode->i_mode = obj->yst_mode;
	inode->i_uid = obj->yst_uid;
	inode->i_gid = obj->yst_gid;
	inode->i_sb	= &yaffs_sb;
	inode->i_size = yaffs_get_obj_length(obj);

	switch (obj->yst_mode & S_IFMT) {
	default:	/* fifo, device or socket */
		// init_special_inode(inode, obj->yst_mode,
				   // old_decode_dev(obj->yst_rdev));
		break;
	case S_IFREG:	/* file */
		// inode->i_op = &yaffs_file_inode_operations;
		// inode->i_fop = &yaffs_file_operations;
		// inode->i_mapping->a_ops = &yaffs_file_address_operations;
		break;
	case S_IFDIR:	/* directory */
		// inode->i_op = &yaffs_dir_inode_operations;
		// inode->i_fop = &yaffs_dir_operations;
		break;
	case S_IFLNK:	/* symlink */
		// inode->i_op = &yaffs_symlink_inode_operations;
		break;
	}
	inode->i_op = &yaffs_op;
	yaffs_inode_to_obj_lv(inode) = obj;
	obj->my_inode = inode;
}

struct inode *iget_locked(struct super_block *sb, unsigned long ino)
{
	struct list_head *i;
	list_for_each(i, (&(sb->s_inodes))){
	//for(i=&(sb->s_inodes);i!=&(sb->s_inodes);i=i->next){
		struct inode *inode = list_entry(i,struct inode,i_list);
		if(inode->i_ino == ino){
			inode->i_nlink++;
			return inode;
		}
	}
	struct inode *inode = (kmalloc)(sizeof(struct inode));
	
	inode->i_state &=I_NEW;
	inode->i_ino = ino;
	inode->i_nlink = 1;
	list_add_tail(&(inode->i_list),&(sb->s_inodes));
	return inode;
}

static struct inode *yaffs_iget(struct super_block *sb, unsigned long ino)
{
	struct inode *inode;
	struct yaffs_obj *obj;
	struct yaffs_dev *dev = yaffs_super_to_dev(sb);

	yaffs_trace(YAFFS_TRACE_OS, "yaffs_iget for %lu", ino);

	inode = iget_locked(sb, ino);
	if (!inode)
		return ERR_PTR(-ENOMEM);
	if (!(inode->i_state & I_NEW))
		return inode;

	/* NB This is called as a side effect of other functions, but
	 * we had to release the lock to prevent deadlocks, so
	 * need to lock again.
	 */

	yaffs_gross_lock(dev);

	obj = yaffs_find_by_number(dev, inode->i_ino);
	yaffs_fill_inode_from_obj(inode, obj);

	yaffs_gross_unlock(dev);
	//TODO:
	//unlock_new_inode(inode);
	return inode;
}
struct inode *yaffs_get_inode(struct super_block *sb, int mode, int dev,
			      struct yaffs_obj *obj)
{
	struct inode *inode;

	if (!sb) {
		yaffs_trace(YAFFS_TRACE_OS,
			"yaffs_get_inode for NULL super_block!!");
		return NULL;

	}

	if (!obj) {
		yaffs_trace(YAFFS_TRACE_OS,
			"yaffs_get_inode for NULL object!!");
		return NULL;

	}

	yaffs_trace(YAFFS_TRACE_OS,
		"yaffs_get_inode for object %d",
		obj->obj_id);

	inode = yaffs_iget(sb, obj->obj_id);
	if (IS_ERR(inode))
		return NULL;

	/* NB Side effect: iget calls back to yaffs_read_inode(). */
	/* iget also increments the inode's i_count */
	/* NB You can't be holding gross_lock or deadlock will happen! */

	return inode;
}

static int yaffs_mknod(struct inode *dir, struct dentry *dentry, int mode,
		       dev_t rdev)
{
	struct inode *inode;
	struct yaffs_obj *obj = NULL;
	struct yaffs_dev *dev;
	struct yaffs_obj *parent = yaffs_inode_to_obj(dir);
	int error;
	// uid_t uid = current->cred->fsuid;
	// gid_t gid =
	   // (dir->i_mode & S_ISGID) ? dir->i_gid : current->cred->fsgid;
	uid_t uid = 0;
	gid_t gid = 0;
	if ((dir->i_mode & S_ISGID) && S_ISDIR(mode))
		mode |= S_ISGID;

	if (!parent) {
		yaffs_trace(YAFFS_TRACE_OS,
			"yaffs_mknod: could not get parent object");
		return -EPERM;
	}

	yaffs_trace(YAFFS_TRACE_OS,
			"yaffs_mknod: parent object %d type %d",
			parent->obj_id, parent->variant_type);

	yaffs_trace(YAFFS_TRACE_OS,
		"yaffs_mknod: making oject for %s, mode %x dev %x",
		dentry->d_name.name, mode, rdev);

	dev = parent->my_dev;

	yaffs_gross_lock(dev);

	if (yaffs_get_n_free_chunks(dev) < 1) {
		error = -ENOSPC;
		goto err_out;
	}

	switch (mode & S_IFMT) {
	default:
		/* Special (socket, fifo, device...) */
		// yaffs_trace(YAFFS_TRACE_OS, "yaffs_mknod: making special");
		// obj =
		    // yaffs_create_special(parent, dentry->d_name.name, mode, uid,
					 // gid, old_encode_dev(rdev));
		break;
	case S_IFREG:		/* file          */
		yaffs_trace(YAFFS_TRACE_OS, "yaffs_mknod: making file");
		obj = yaffs_create_file(parent, dentry->d_name.name, mode, uid,
					gid);
		break;
	case S_IFDIR:		/* directory */
		yaffs_trace(YAFFS_TRACE_OS, "yaffs_mknod: making directory");
		obj = yaffs_create_dir(parent, dentry->d_name.name, mode,
				       uid, gid);
		break;
	case S_IFLNK:		/* symlink */
		yaffs_trace(YAFFS_TRACE_OS, "yaffs_mknod: making symlink");
		obj = NULL;	/* Do we ever get here? */
		break;
	}

	if (!obj) {
		error = -ENOMEM;
		goto err_out;
	}

	/* Can not call yaffs_get_inode() with gross lock held */
	yaffs_gross_unlock(dev);


	inode = yaffs_get_inode(dir->i_sb, mode, rdev, obj);
	//d_instantiate(dentry, inode);
	update_dir_time(dir);
	// yaffs_trace(YAFFS_TRACE_OS,
		// "yaffs_mknod created object %d count = %d",
		// obj->obj_id, atomic_read(&inode->i_count));
	yaffs_fill_inode_from_obj(dir, parent);
	return 0;

err_out:
	yaffs_gross_unlock(dev);
	yaffs_trace(YAFFS_TRACE_OS, "yaffs_mknod error %d", error);
	return error;

}
//TODO
struct inode *yaffs2_get_parent(struct dentry *dentry)
{

	struct super_block *sb = dentry->d_inode->i_sb;
	struct dentry *parent = ERR_PTR(-ENOENT);
	struct inode *inode;
	unsigned long parent_ino;
	struct yaffs_obj *d_obj;
	struct yaffs_obj *parent_obj;

	d_obj = yaffs_inode_to_obj(dentry->d_inode);

	if (d_obj) {
		parent_obj = d_obj->parent;
		if (parent_obj) {
			parent_ino = yaffs_get_obj_inode(parent_obj);
			inode = yaffs_iget(sb, parent_ino);

			if (IS_ERR(inode)) {
				parent = ERR_CAST(inode);
				return inode;
			} else {
				// parent = d_obtain_alias(inode);
				// if (!IS_ERR(parent)) {
					// parent = ERR_PTR(-ENOMEM);
					// iput(inode);
				// }
			}
		}else{//TODO
			yaffs_trace(YAFFS_TRACE_OS,"yaffs2_get_parent parent_obj = NULL");
		}
	}
	return NULL;
}
static struct dentry *yaffs_lookup(struct inode *dir, struct dentry *dentry,
				   struct nameidata *n)
{
	struct yaffs_obj *obj;
	struct inode *inode = NULL;
	struct yaffs_dev *dev = yaffs_inode_to_obj(dir)->my_dev;

	// if (current != yaffs_dev_to_lc(dev)->readdir_process)
		// yaffs_gross_lock(dev);

	yaffs_trace(YAFFS_TRACE_OS,
		"yaffs_lookup for %d:%s",
		yaffs_inode_to_obj(dir)->obj_id, dentry->d_name.name);

	obj = yaffs_find_by_name(yaffs_inode_to_obj(dir), dentry->d_name.name);

	obj = yaffs_get_equivalent_obj(obj);	/* in case it was a hardlink */

	/* Can't hold gross lock when calling yaffs_get_inode() */
	// if (current != yaffs_dev_to_lc(dev)->readdir_process)
		// yaffs_gross_unlock(dev);

	if (obj) {
		yaffs_trace(YAFFS_TRACE_OS,
			"yaffs_lookup found %d", obj->obj_id);

		inode = yaffs_get_inode(dir->i_sb, obj->yst_mode, 0, obj);

		if (inode) {
			yaffs_trace(YAFFS_TRACE_OS, "yaffs_loookup dentry");
			//d_add(dentry, inode);
			dentry->d_inode = inode;
			/* return dentry; */
			//return NULL;
			return dentry;
		}

	} else {
		yaffs_trace(YAFFS_TRACE_OS, "yaffs_lookup not found");

	}

	//d_add(dentry, inode);

	return NULL;
}

static int yaffs_mkdir(struct inode *dir, struct dentry *dentry, int mode)
{
	return yaffs_mknod(dir, dentry, mode | S_IFDIR, 0);
}

static int yaffs_create(struct inode *dir, struct dentry *dentry, int mode,
			struct nameidata *n)
{
	return yaffs_mknod(dir, dentry, mode | S_IFREG, 0);
}

/*
 * The VFS layer already does all the dentry stuff for rename.
 *
 * NB: POSIX says you can rename an object over an old object of the same name
 */
static int yaffs_rename(struct inode *old_dir, struct dentry *old_dentry,
			struct inode *new_dir, struct dentry *new_dentry)
{
	struct yaffs_dev *dev;
	int ret_val = YAFFS_FAIL;
	struct yaffs_obj *target;

	yaffs_trace(YAFFS_TRACE_OS, "yaffs_rename");
	dev = yaffs_inode_to_obj(old_dir)->my_dev;

	yaffs_gross_lock(dev);

	/* Check if the target is an existing directory that is not empty. */
	target = yaffs_find_by_name(yaffs_inode_to_obj(new_dir),
				    new_dentry->d_name.name);

	if (target && target->variant_type == YAFFS_OBJECT_TYPE_DIRECTORY &&
	    !list_empty(&target->variant.dir_variant.children)) {

		yaffs_trace(YAFFS_TRACE_OS, "target is non-empty dir");

		ret_val = YAFFS_FAIL;
	} else {
		/* Now does unlinking internally using shadowing mechanism */
		yaffs_trace(YAFFS_TRACE_OS, "calling yaffs_rename_obj");

		ret_val = yaffs_rename_obj(yaffs_inode_to_obj(old_dir),
					   old_dentry->d_name.name,
					   yaffs_inode_to_obj(new_dir),
					   new_dentry->d_name.name);
	}
	yaffs_gross_unlock(dev);

	if (ret_val == YAFFS_OK) {
		if (target)
			inode_dec_link_count(new_dentry->d_inode);

		update_dir_time(old_dir);
		if (old_dir != new_dir)
			update_dir_time(new_dir);
		return 0;
	} else {
		return -ENOTEMPTY;
	}
}

static int yaffs_unlink(struct inode *dir, struct dentry *dentry)
{
	int ret_val;
	struct yaffs_dev *dev;
	struct yaffs_obj *obj;

	yaffs_trace(YAFFS_TRACE_OS,
		"yaffs_unlink %d:%s",
		(int)(dir->i_ino), dentry->d_name.name);
	obj = yaffs_inode_to_obj(dir);
	dev = obj->my_dev;

	yaffs_gross_lock(dev);

	ret_val = yaffs_unlinker(obj, dentry->d_name.name);

	if (ret_val == YAFFS_OK) {
		inode_dec_link_count(dentry->d_inode);
		dir->i_version++;
		yaffs_gross_unlock(dev);
		update_dir_time(dir);
		return 0;
	}
	yaffs_gross_unlock(dev);
	return -ENOTEMPTY;
}
static int yaffs_setattr(struct dentry *dentry, struct iattr *attr)
{
	struct inode *inode = dentry->d_inode;
	int error = 0;
	struct yaffs_dev *dev;
	int result;

	yaffs_trace(YAFFS_TRACE_OS,
		"yaffs_setattr of object %d",
		yaffs_inode_to_obj(inode)->obj_id);

	/* Fail if a requested resize >= 2GB */
	if (attr->ia_valid & ATTR_SIZE && (attr->ia_size >> 31))
		error = -EINVAL;

	// if (!error)
		// error = inode_change_ok(inode, attr);

	if (!error) {
		//TODO:
		//setattr_copy(inode, attr);
		if (attr->ia_valid & ATTR_MODE)
			inode->i_mode = attr->ia_mode;
		yaffs_trace(YAFFS_TRACE_OS, "inode_setattr called");
		if (attr->ia_valid & ATTR_SIZE) {
			//TODO:
			//truncate_setsize(inode, attr->ia_size);
			inode->i_size = attr->ia_size;
			inode->i_blocks = (inode->i_size + 511) >> 9;
		}
		dev = yaffs_inode_to_obj(inode)->my_dev;
		if (attr->ia_valid & ATTR_SIZE) {
			yaffs_trace(YAFFS_TRACE_OS, "resize to %d(%x)",
					   (int)(attr->ia_size),
					   (int)(attr->ia_size));
		}

		yaffs_gross_lock(dev);
		result = yaffs_set_attribs(yaffs_inode_to_obj(inode), attr);
		if (result != YAFFS_OK)
			error = -EPERM;
		yaffs_gross_unlock(dev);
	}

	yaffs_trace(YAFFS_TRACE_OS, "yaffs_setattr done returning %d", error);

	return error;
}

static int yaffs_setxattr(struct dentry *dentry, const char *name,
		   const void *value, size_t size, int flags)
{
	struct inode *inode = dentry->d_inode;
	int error;
	struct yaffs_dev *dev;
	struct yaffs_obj *obj = yaffs_inode_to_obj(inode);

	yaffs_trace(YAFFS_TRACE_OS, "yaffs_setxattr of object %d", obj->obj_id);

	dev = obj->my_dev;
	yaffs_gross_lock(dev);
	error = yaffs_set_xattrib(obj, name, value, size, flags);
	yaffs_gross_unlock(dev);

	yaffs_trace(YAFFS_TRACE_OS, "yaffs_setxattr done returning %d", error);

	return error;
}

static ssize_t yaffs_getxattr(struct dentry *dentry, const char *name,
				void *buff, size_t size)
{
	struct inode *inode = dentry->d_inode;
	int error;
	struct yaffs_dev *dev;
	struct yaffs_obj *obj = yaffs_inode_to_obj(inode);

	yaffs_trace(YAFFS_TRACE_OS,
		"yaffs_getxattr \"%s\" from object %d",
		name, obj->obj_id);

	dev = obj->my_dev;
	yaffs_gross_lock(dev);
	error = yaffs_get_xattrib(obj, name, buff, size);
	yaffs_gross_unlock(dev);

	yaffs_trace(YAFFS_TRACE_OS, "yaffs_getxattr done returning %d", error);

	return error;
}

static int yaffs_removexattr(struct dentry *dentry, const char *name)
{
	struct inode *inode = dentry->d_inode;
	int error;
	struct yaffs_dev *dev;
	struct yaffs_obj *obj = yaffs_inode_to_obj(inode);

	yaffs_trace(YAFFS_TRACE_OS,
		"yaffs_removexattr of object %d", obj->obj_id);

	dev = obj->my_dev;
	yaffs_gross_lock(dev);
	error = yaffs_remove_xattrib(obj, name);
	yaffs_gross_unlock(dev);

	yaffs_trace(YAFFS_TRACE_OS,
		"yaffs_removexattr done returning %d", error);

	return error;
}

static ssize_t yaffs_listxattr(struct dentry *dentry, char *buff, size_t size)
{
	struct inode *inode = dentry->d_inode;
	int error;
	struct yaffs_dev *dev;
	struct yaffs_obj *obj = yaffs_inode_to_obj(inode);

	yaffs_trace(YAFFS_TRACE_OS,
		"yaffs_listxattr of object %d", obj->obj_id);

	dev = obj->my_dev;
	yaffs_gross_lock(dev);
	error = yaffs_list_xattrib(obj, buff, size);
	yaffs_gross_unlock(dev);

	yaffs_trace(YAFFS_TRACE_OS,
		"yaffs_listxattr done returning %d", error);

	return error;
}

static ssize_t yaffs_file_write(struct file *f, const char *buf, size_t n,
				loff_t *pos)
{
	struct yaffs_obj *obj;
	int n_written, ipos;
	struct inode *inode;
	struct yaffs_dev *dev;
	//TODO: n_written 32位  loff_t *pos 64位 可能会出错
	obj = yaffs_dentry_to_obj(f->f_dentry);

	if (!obj) {
		/* This should not happen */
		yaffs_trace(YAFFS_TRACE_OS,
			"yaffs_file_write: hey obj is null!");
		return -EINVAL;
	}

	dev = obj->my_dev;

	yaffs_gross_lock(dev);

	inode = f->f_dentry->d_inode;

	if (!S_ISBLK(inode->i_mode) && f->f_flags & O_APPEND)
		ipos = inode->i_size;
	else
		ipos = *pos;

	yaffs_trace(YAFFS_TRACE_OS,
		"yaffs_file_write about to write writing %u(%x) bytes to object %d at %d(%x)",
		(unsigned)n, (unsigned)n, obj->obj_id, ipos, ipos);

	n_written = yaffs_wr_file(obj, buf, ipos, n, 0);

	//yaffs_touch_super(dev);

	yaffs_trace(YAFFS_TRACE_OS,
		"yaffs_file_write: %d(%x) bytes written",
		(unsigned)n, (unsigned)n);

	if (n_written > 0) {
		ipos += n_written;
		*pos = ipos;
		if (ipos > inode->i_size) {
			inode->i_size = ipos;
			inode->i_blocks = (ipos + 511) >> 9;

			yaffs_trace(YAFFS_TRACE_OS,
				"yaffs_file_write size updated to %d bytes, %d blocks",
				ipos, (int)(inode->i_blocks));
		}
	}
	yaffs_gross_unlock(dev);
	return (n_written == 0) && (n > 0) ? -ENOSPC : n_written;
}
static int yaffs_file_read(struct file *f, const char *buf, size_t n,
				loff_t *pos)
{
	struct yaffs_obj *obj;
	int n_read, ipos;
	struct inode *inode;
	struct yaffs_dev *dev;
	//TODO: n_read 32位  loff_t *pos 64位 可能会出错
	obj = yaffs_dentry_to_obj(f->f_dentry);

	if (!obj) {
		/* This should not happen */
		yaffs_trace(YAFFS_TRACE_OS,
			"yaffs_file_read: hey obj is null!");
		return -EINVAL;
	}

	dev = obj->my_dev;

	yaffs_gross_lock(dev);

	inode = f->f_dentry->d_inode;

	if (yaffs_get_obj_length(obj) <= *pos)
		return -EOVERFLOW;
	else 
		ipos = *pos;

	yaffs_trace(YAFFS_TRACE_OS,
		"yaffs_file_read about to read reading %u(%x) bytes from object %d at %d(%x)",
		(unsigned)n, (unsigned)n, obj->obj_id, ipos, ipos);

	n_read = yaffs_file_rd(obj, buf, ipos, n);
	
	//yaffs_touch_super(dev);

	yaffs_trace(YAFFS_TRACE_OS,
		"yaffs_file_read: %d(%x) bytes read",
		(unsigned)n, (unsigned)n);

	if (n_read >= 0){
		ipos += n_read;
		*pos = ipos;
	}	
	yaffs_gross_unlock(dev);
	return (n_read == 0) && (n > 0) ? -EINVAL : n_read;
}
/* yaffs_evict_inode combines into one operation what was previously done in
 * yaffs_clear_inode() and yaffs_delete_inode()
 *
 */
static void yaffs_evict_inode(struct inode *inode)
{
	struct yaffs_obj *obj;
	struct yaffs_dev *dev;
	int deleteme = 0;

	obj = yaffs_inode_to_obj(inode);

	// yaffs_trace(YAFFS_TRACE_OS,
		// "yaffs_evict_inode: ino %d, count %d %s",
		// (int)inode->i_ino,
		// atomic_read(&inode->i_count),
		// obj ? "object exists" : "null object");

	// if (!inode->i_nlink && !is_bad_inode(inode))
		// deleteme = 1;
	// truncate_inode_pages(&inode->i_data, 0);
	// end_writeback(inode);
	if (!inode->i_nlink)
		deleteme = 1;
	if (deleteme && obj) {
		dev = obj->my_dev;
		yaffs_gross_lock(dev);
		yaffs_del_obj(obj);
		yaffs_gross_unlock(dev);
	}
	list_del(&(inode->i_list));
	memset(inode, 0, sizeof(struct inode));//TODO:
	(kfree)(inode);
	// if (obj) {
		// dev = obj->my_dev;
		// yaffs_gross_lock(dev);
		// yaffs_unstitch_obj(inode, obj);
		// yaffs_gross_unlock(dev);
	// }
	//TODO:释放inode
}

/*-----------------------------------------------------------------*/
/* Directory search context allows us to unlock access to yaffs during
 * filldir without causing problems with the directory being modified.
 * This is similar to the tried and tested mechanism used in yaffs direct.
 *
 * A search context iterates along a doubly linked list of siblings in the
 * directory. If the iterating object is deleted then this would corrupt
 * the list iteration, likely causing a crash. The search context avoids
 * this by using the remove_obj_fn to move the search context to the
 * next object before the object is deleted.
 *
 * Many readdirs (and thus seach conexts) may be alive simulateously so
 * each struct yaffs_dev has a list of these.
 *
 * A seach context lives for the duration of a readdir.
 *
 * All these functions must be called while yaffs is locked.
 */

struct yaffs_search_context {
	struct yaffs_dev *dev;
	struct yaffs_obj *dir_obj;
	struct yaffs_obj *next_return;
	struct list_head others;
};

/*
 * yaffs_new_search() creates a new search context, initialises it and
 * adds it to the device's search context list.
 *
 * Called at start of readdir.
 */
static struct yaffs_search_context *yaffs_new_search(struct yaffs_obj *dir)
{
	struct yaffs_dev *dev = dir->my_dev;
	struct yaffs_search_context *sc =
	    kmalloc(sizeof(struct yaffs_search_context), GFP_NOFS);

	if (!sc)
		return NULL;

	sc->dir_obj = dir;
	sc->dev = dev;
	if (list_empty(&sc->dir_obj->variant.dir_variant.children))
		sc->next_return = NULL;
	else
		sc->next_return =
		    list_entry(dir->variant.dir_variant.children.next,
			       struct yaffs_obj, siblings);
	INIT_LIST_HEAD(&sc->others);
	//list_add(&sc->others, &(yaffs_dev_to_lc(dev)->search_contexts));

	return sc;
}

/*
 * yaffs_search_end() disposes of a search context and cleans up.
 */
static void yaffs_search_end(struct yaffs_search_context *sc)
{
	if (sc) {
		list_del(&sc->others);
		kfree(sc);
	}
}

/*
 * yaffs_search_advance() moves a search context to the next object.
 * Called when the search iterates or when an object removal causes
 * the search context to be moved to the next object.
 */
static void yaffs_search_advance(struct yaffs_search_context *sc)
{
	if (!sc)
		return;

	if (sc->next_return == NULL ||
	    list_empty(&sc->dir_obj->variant.dir_variant.children))
		sc->next_return = NULL;
	else {
		struct list_head *next = sc->next_return->siblings.next;

		if (next == &sc->dir_obj->variant.dir_variant.children)
			sc->next_return = NULL;	/* end of list */
		else
			sc->next_return =
			    list_entry(next, struct yaffs_obj, siblings);
	}
}


static int yaffs_readdir(struct file *f, void *dirent, filldir_t filldir)
{
	struct yaffs_obj *obj;
	struct yaffs_dev *dev;
	struct yaffs_search_context *sc;
	struct inode *inode = f->f_dentry->d_inode;
	unsigned long offset, curoffs;
	struct yaffs_obj *l;
	int ret_val = 0;
	char name[YAFFS_MAX_NAME_LENGTH + 1];

	obj = yaffs_dentry_to_obj(f->f_dentry);
	dev = obj->my_dev;

	yaffs_gross_lock(dev);

	//yaffs_dev_to_lc(dev)->readdir_process = current;

	offset = f->f_pos;
	int start_offset = offset;
	
	sc = yaffs_new_search(obj);
	if (!sc) {
		ret_val = -ENOMEM;
		goto out;
	}

	yaffs_trace(YAFFS_TRACE_OS,
		"yaffs_readdir: starting at %d", (int)offset);

	if (offset == 0) {
		yaffs_trace(YAFFS_TRACE_OS,
			"yaffs_readdir: entry . ino %d",
			(int)inode->i_ino);
		yaffs_gross_unlock(dev);
		if (filldir(dirent, ".", 1, offset, inode->i_ino, DT_DIR) < 0) {
			yaffs_gross_lock(dev);
			goto out;
		}
		yaffs_gross_lock(dev);
		offset++;
		f->f_pos++;
	}
	if (offset == 1) {
		struct inode *inode = NULL;
		if(lookup(f->f_dentry->d_inode, "..", &inode))
			goto out;
		yaffs_trace(YAFFS_TRACE_OS,
			"yaffs_readdir: entry .. ino %d",
			//(int)f->f_dentry->d_parent->d_inode->i_ino);
			inode->i_ino);
		yaffs_gross_unlock(dev);
		
		if (filldir(dirent, "..", 2, offset,
		//	    f->f_dentry->d_parent->d_inode->i_ino,
			    inode->i_ino,
			    DT_DIR) < 0) {
			yaffs_gross_lock(dev);
			goto out;
		}
		yaffs_gross_lock(dev);
		offset++;
		f->f_pos++;
	}

	curoffs = 1;

	/* If the directory has changed since the open or last call to
	   readdir, rewind to after the 2 canned entries. */
	   
	//TODO:暂时没有处理i_version
	
	
	// if (f->f_version != inode->i_version) {
		// offset = 2;
		// f->f_pos = offset;
		// f->f_version = inode->i_version;
	// }

	while (sc->next_return) {
		curoffs++;
		l = sc->next_return;
		if (curoffs >= offset) {
			int this_inode = yaffs_get_obj_inode(l);
			int this_type = yaffs_get_obj_type(l);

			yaffs_get_obj_name(l, name, YAFFS_MAX_NAME_LENGTH + 1);
			yaffs_trace(YAFFS_TRACE_OS,
				"yaffs_readdir: %s inode %d",
				name, yaffs_get_obj_inode(l));

			yaffs_gross_unlock(dev);

			if (filldir(dirent,
				    name,
				    strlen(name),
				    offset, this_inode, this_type) < 0) {
				yaffs_gross_lock(dev);
				goto out;
			}

			yaffs_gross_lock(dev);

			offset++;
			f->f_pos++;
		}
		yaffs_search_advance(sc);
	}
	
	offset = start_offset;//使ret_val为0
out:
	printf("readdir out\n");
	ret_val = offset - start_offset;
	yaffs_search_end(sc);
	//yaffs_dev_to_lc(dev)->readdir_process = NULL;
	yaffs_gross_unlock(dev);

	return ret_val;
}

static int lookup(struct inode *inode, char *filename, struct inode **res)
{
	struct dentry dentry;
	strcpy(dentry.d_name.name,filename);
	yaffs_trace(YAFFS_TRACE_OS,"lookup, name = %s",filename);
	if(strcmp(filename,".")==0){
		if(res)
			*res = inode;
	}else if(strcmp(filename,"..")==0){
		if(inode == root_inode){
			if(res)
				*res = inode;
		}else{
			dentry.d_inode = inode;
			if(res){
				struct inode *temp = yaffs2_get_parent(&dentry);
				if(temp){
					*res = temp;
				}else{
					return -1;
				}
			}
		}
	}else if(yaffs_lookup(inode, &dentry,NULL)){
		if(res){
			if(!dentry.d_inode)
				return -1;//应该不会到这
			*res = dentry.d_inode;
		}
	}else{
		return -1;
	}
	printf("lookup end!\n");
	return 0;
}

static int mknod(struct inode *inode, char *name,mode_t mode,dev_t dev){
	struct dentry dentry;
	strcpy(dentry.d_name.name,name);
	yaffs_trace(YAFFS_TRACE_OS,"mknod, name = %s",name);
	return yaffs_mknod(inode, &dentry, mode, dev);
}
static int release(struct file *f){
	struct yaffs_obj *obj = NULL;
	struct yaffs_dev *dev = NULL;

	int retVal = -1;
	obj = yaffs_dentry_to_obj(f->f_dentry);

	if (!obj)
		return -EBADF;
	else {
		/* clean up */
		if(!((f->f_inode->i_mode & S_IFMT )& S_IFDIR))
			yaffs_flush_file(obj, 1, 0, 0);
		retVal = 0;
	}
	return retVal;
}

static int unlink(struct inode *dir,char *name){
	struct dentry dentry;
	strcpy(dentry.d_name.name,name);
	yaffs_trace(YAFFS_TRACE_OS,"unlink, name = %s",name);
	return yaffs_unlink(dir, &dentry);
}

static int mkdir(struct inode *inode, char *name,mode_t mode){
	struct dentry dentry;
	strcpy(dentry.d_name.name,name);
	yaffs_trace(YAFFS_TRACE_OS,"mkdir, name = %s",name);
	return yaffs_mkdir(inode, &dentry, mode);
}
static int rename(struct inode *old_inode,char *old_name,struct inode *new_inode,char *new_name){
	struct dentry old_dentry,new_dentry;
	strcpy(old_dentry.d_name.name,old_name);
	strcpy(new_dentry.d_name.name,new_name);
	yaffs_trace(YAFFS_TRACE_OS,"rename, old_name = %s , new_name = %s", old_name, new_name);
	return yaffs_rename(old_inode, &old_dentry, new_inode, &new_dentry);
}
struct fs_operation yaffs_op = {
	.evict_inode = yaffs_evict_inode,
	.lookup = lookup,
	.mknod = mknod,
	.file_read = yaffs_file_read,
	.file_write = yaffs_file_write,
	.file_release = release,
	.unlink = unlink,
	.rename = rename,
	.mkdir = mkdir,
	.file_readdir = yaffs_readdir,
};
extern struct yaffs_dev nand_dev;
void fs_test(){
	struct dentry dentry = {
		.d_name.name="test.txt"
	};
	struct dentry dentry2 = {
		.d_name.name="test2.txt"
	};
	struct dentry dentry3 = {
		.d_name.name="test3"
	};
	
	
	struct inode *inode = get_root_inode();
	struct dentry *ret = yaffs_lookup(inode,&dentry,NULL);
	if(ret==NULL)
		goto err;
	if((!yaffs_lookup(inode,&dentry2,NULL))&&IS_ERR(yaffs_create(inode,&dentry2,S_IRWXU|S_IRWXG|S_TRWXO,NULL)))
		goto err;
	if((!yaffs_lookup(inode,&dentry3,NULL))&&IS_ERR(yaffs_mkdir(inode,&dentry3,S_IRWXU|S_IRWXG|S_TRWXO)))
		goto err;
	printf("fs test succeed!");
	return;
	err:
		printf("fs test failed!");
}

struct inode *get_root_inode(){
	if(root_inode)
		return root_inode;
	else
		return root_inode = yaffs_get_inode(&yaffs_sb,0,0,yaffs_root(&nand_dev));
}
#include "yaffsfs.h"
void init_yaffs_fs(){
	extern unsigned int yaffs_trace_mask;
	//yaffs_trace_mask = YAFFS_TRACE_OS|YAFFS_TRACE_ERROR|YAFFS_TRACE_ERROR|YAFFS_TRACE_WRITE;
	yaffs_trace_mask = YAFFS_TRACE_OS;
	INIT_LIST_HEAD(&(yaffs_sb.s_inodes));
	INIT_LIST_HEAD(&(yaffs_sb.s_files));
	yaffs_sb.s_fs_info = &nand_dev;
	printf("Starting up the file system...\n");
	yaffs_start_up();
	printf("The file system is being mounted...\n");
	yaffs_mount("/");
	root_inode = yaffs_get_inode(&yaffs_sb,0,0,yaffs_root(&nand_dev));
}