/*
 * Copyright (c) 2000-2002,2005-2006 Silicon Graphics, Inc.
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it would be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write the Free Software Foundation,
 * Inc.,  51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include "xfs.h"
#include "xfs_vnodeops.h"
#include "xfs_bmap_btree.h"
#include "xfs_inode.h"
#include "xfs_trace.h"

/*
 * note: all filemap functions return negative error codes. These
 * need to be inverted before returning to the xfs core functions.
 */
void
xfs_tosspages(
	xfs_inode_t	*ip,
	xfs_off_t	first,
	xfs_off_t	last,
	int		fiopt)
{
	/* can't toss partial tail pages, so mask them out */
	last &= ~(PAGE_SIZE - 1);
	truncate_inode_pages_range(VFS_I(ip)->i_mapping, first, last - 1);
}

int
xfs_flushinval_pages(
	xfs_inode_t	*ip,
	xfs_off_t	first,
	xfs_off_t	last,
	int		fiopt)
{
	struct address_space *mapping = VFS_I(ip)->i_mapping;
	int		ret = 0;

	trace_xfs_pagecache_inval(ip, first, last);

	xfs_iflags_clear(ip, XFS_ITRUNCATED);
	ret = filemap_write_and_wait_range(mapping, first,
				last == -1 ? LLONG_MAX : last);
	if (!ret)
		truncate_inode_pages_range(mapping, first, last);
	return -ret;
}

int
xfs_flush_pages(
	xfs_inode_t	*ip,
	xfs_off_t	first,
	xfs_off_t	last,
	uint64_t	flags,
	int		fiopt)
{
	struct address_space *mapping = VFS_I(ip)->i_mapping;
	int		ret = 0;
	int		ret2;

	xfs_iflags_clear(ip, XFS_ITRUNCATED);
	ret = -filemap_fdatawrite_range(mapping, first,
				last == -1 ? LLONG_MAX : last);
	if (flags & XBF_ASYNC)
		return ret;
	ret2 = xfs_wait_on_pages(ip, first, last);
	if (!ret)
		ret = ret2;
	return ret;
}

int
xfs_wait_on_pages(
	xfs_inode_t	*ip,
	xfs_off_t	first,
	xfs_off_t	last)
{
	struct address_space *mapping = VFS_I(ip)->i_mapping;

	if (mapping_tagged(mapping, PAGECACHE_TAG_WRITEBACK)) {
		return -filemap_fdatawait_range(mapping, first,
					last == -1 ? ip->i_size - 1 : last);
	}
	return 0;
}
