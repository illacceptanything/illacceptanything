#ifndef _FS_CEPH_LIBCEPH_H
#define _FS_CEPH_LIBCEPH_H

#include "ceph_debug.h"

#include <asm/unaligned.h>
#include <linux/backing-dev.h>
#include <linux/completion.h>
#include <linux/exportfs.h>
#include <linux/fs.h>
#include <linux/mempool.h>
#include <linux/pagemap.h>
#include <linux/wait.h>
#include <linux/writeback.h>
#include <linux/slab.h>

#include "types.h"
#include "messenger.h"
#include "msgpool.h"
#include "mon_client.h"
#include "osd_client.h"
#include "ceph_fs.h"

/*
 * Supported features
 */
#define CEPH_FEATURE_SUPPORTED_DEFAULT CEPH_FEATURE_NOSRCADDR
#define CEPH_FEATURE_REQUIRED_DEFAULT  CEPH_FEATURE_NOSRCADDR

/*
 * mount options
 */
#define CEPH_OPT_FSID             (1<<0)
#define CEPH_OPT_NOSHARE          (1<<1) /* don't share client with other sbs */
#define CEPH_OPT_MYIP             (1<<2) /* specified my ip */
#define CEPH_OPT_NOCRC            (1<<3) /* no data crc on writes */

#define CEPH_OPT_DEFAULT   (0);

#define ceph_set_opt(client, opt) \
	(client)->options->flags |= CEPH_OPT_##opt;
#define ceph_test_opt(client, opt) \
	(!!((client)->options->flags & CEPH_OPT_##opt))

struct ceph_options {
	int flags;
	struct ceph_fsid fsid;
	struct ceph_entity_addr my_addr;
	int mount_timeout;
	int osd_idle_ttl;
	int osd_timeout;
	int osd_keepalive_timeout;

	/*
	 * any type that can't be simply compared or doesn't need need
	 * to be compared should go beyond this point,
	 * ceph_compare_options() should be updated accordingly
	 */

	struct ceph_entity_addr *mon_addr; /* should be the first
					      pointer type of args */
	int num_mon;
	char *name;
	char *secret;
};

/*
 * defaults
 */
#define CEPH_MOUNT_TIMEOUT_DEFAULT  60
#define CEPH_OSD_TIMEOUT_DEFAULT    60  /* seconds */
#define CEPH_OSD_KEEPALIVE_DEFAULT  5
#define CEPH_OSD_IDLE_TTL_DEFAULT    60
#define CEPH_MOUNT_RSIZE_DEFAULT    (512*1024) /* readahead */

#define CEPH_MSG_MAX_FRONT_LEN	(16*1024*1024)
#define CEPH_MSG_MAX_DATA_LEN	(16*1024*1024)

#define CEPH_AUTH_NAME_DEFAULT   "guest"

/*
 * Delay telling the MDS we no longer want caps, in case we reopen
 * the file.  Delay a minimum amount of time, even if we send a cap
 * message for some other reason.  Otherwise, take the oppotunity to
 * update the mds to avoid sending another message later.
 */
#define CEPH_CAPS_WANTED_DELAY_MIN_DEFAULT      5  /* cap release delay */
#define CEPH_CAPS_WANTED_DELAY_MAX_DEFAULT     60  /* cap release delay */

#define CEPH_CAP_RELEASE_SAFETY_DEFAULT        (CEPH_CAPS_PER_RELEASE * 4)

/* mount state */
enum {
	CEPH_MOUNT_MOUNTING,
	CEPH_MOUNT_MOUNTED,
	CEPH_MOUNT_UNMOUNTING,
	CEPH_MOUNT_UNMOUNTED,
	CEPH_MOUNT_SHUTDOWN,
};

/*
 * subtract jiffies
 */
static inline unsigned long time_sub(unsigned long a, unsigned long b)
{
	BUG_ON(time_after(b, a));
	return (long)a - (long)b;
}

struct ceph_mds_client;

/*
 * per client state
 *
 * possibly shared by multiple mount points, if they are
 * mounting the same ceph filesystem/cluster.
 */
struct ceph_client {
	struct ceph_fsid fsid;
	bool have_fsid;

	void *private;

	struct ceph_options *options;

	struct mutex mount_mutex;      /* serialize mount attempts */
	wait_queue_head_t auth_wq;
	int auth_err;

	int (*extra_mon_dispatch)(struct ceph_client *, struct ceph_msg *);

	u32 supported_features;
	u32 required_features;

	struct ceph_messenger *msgr;   /* messenger instance */
	struct ceph_mon_client monc;
	struct ceph_osd_client osdc;

#ifdef CONFIG_DEBUG_FS
	struct dentry *debugfs_dir;
	struct dentry *debugfs_monmap;
	struct dentry *debugfs_osdmap;
#endif
};



/*
 * snapshots
 */

/*
 * A "snap context" is the set of existing snapshots when we
 * write data.  It is used by the OSD to guide its COW behavior.
 *
 * The ceph_snap_context is refcounted, and attached to each dirty
 * page, indicating which context the dirty data belonged when it was
 * dirtied.
 */
struct ceph_snap_context {
	atomic_t nref;
	u64 seq;
	int num_snaps;
	u64 snaps[];
};

static inline struct ceph_snap_context *
ceph_get_snap_context(struct ceph_snap_context *sc)
{
	/*
	printk("get_snap_context %p %d -> %d\n", sc, atomic_read(&sc->nref),
	       atomic_read(&sc->nref)+1);
	*/
	if (sc)
		atomic_inc(&sc->nref);
	return sc;
}

static inline void ceph_put_snap_context(struct ceph_snap_context *sc)
{
	if (!sc)
		return;
	/*
	printk("put_snap_context %p %d -> %d\n", sc, atomic_read(&sc->nref),
	       atomic_read(&sc->nref)-1);
	*/
	if (atomic_dec_and_test(&sc->nref)) {
		/*printk(" deleting snap_context %p\n", sc);*/
		kfree(sc);
	}
}

/*
 * calculate the number of pages a given length and offset map onto,
 * if we align the data.
 */
static inline int calc_pages_for(u64 off, u64 len)
{
	return ((off+len+PAGE_CACHE_SIZE-1) >> PAGE_CACHE_SHIFT) -
		(off >> PAGE_CACHE_SHIFT);
}

/* ceph_common.c */
extern const char *ceph_msg_type_name(int type);
extern int ceph_check_fsid(struct ceph_client *client, struct ceph_fsid *fsid);
extern struct kmem_cache *ceph_inode_cachep;
extern struct kmem_cache *ceph_cap_cachep;
extern struct kmem_cache *ceph_dentry_cachep;
extern struct kmem_cache *ceph_file_cachep;

extern int ceph_parse_options(struct ceph_options **popt, char *options,
			      const char *dev_name, const char *dev_name_end,
			      int (*parse_extra_token)(char *c, void *private),
			      void *private);
extern void ceph_destroy_options(struct ceph_options *opt);
extern int ceph_compare_options(struct ceph_options *new_opt,
				struct ceph_client *client);
extern struct ceph_client *ceph_create_client(struct ceph_options *opt,
					      void *private);
extern u64 ceph_client_id(struct ceph_client *client);
extern void ceph_destroy_client(struct ceph_client *client);
extern int __ceph_open_session(struct ceph_client *client,
			       unsigned long started);
extern int ceph_open_session(struct ceph_client *client);

/* pagevec.c */
extern void ceph_release_page_vector(struct page **pages, int num_pages);

extern struct page **ceph_get_direct_page_vector(const char __user *data,
						 int num_pages);
extern void ceph_put_page_vector(struct page **pages, int num_pages);
extern void ceph_release_page_vector(struct page **pages, int num_pages);
extern struct page **ceph_alloc_page_vector(int num_pages, gfp_t flags);
extern int ceph_copy_user_to_page_vector(struct page **pages,
					 const char __user *data,
					 loff_t off, size_t len);
extern int ceph_copy_to_page_vector(struct page **pages,
				    const char *data,
				    loff_t off, size_t len);
extern int ceph_copy_from_page_vector(struct page **pages,
				    char *data,
				    loff_t off, size_t len);
extern int ceph_copy_page_vector_to_user(struct page **pages, char __user *data,
				    loff_t off, size_t len);
extern void ceph_zero_page_vector_range(int off, int len, struct page **pages);


#endif /* _FS_CEPH_SUPER_H */
