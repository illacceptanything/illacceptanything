/*
 * Copyright (c) 2008, Christoph Hellwig
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
#include "xfs_format.h"
#include "xfs_log_format.h"
#include "xfs_trans_resv.h"
#include "xfs_mount.h"
#include "xfs_inode.h"
#include "xfs_quota.h"
#include "xfs_trans.h"
#include "xfs_qm.h"
#include <linux/quota.h>


STATIC int
xfs_quota_type(int type)
{
	switch (type) {
	case USRQUOTA:
		return XFS_DQ_USER;
	case GRPQUOTA:
		return XFS_DQ_GROUP;
	default:
		return XFS_DQ_PROJ;
	}
}

STATIC int
xfs_fs_get_xstate(
	struct super_block	*sb,
	struct fs_quota_stat	*fqs)
{
	struct xfs_mount	*mp = XFS_M(sb);

	if (!XFS_IS_QUOTA_RUNNING(mp))
		return -ENOSYS;
	return xfs_qm_scall_getqstat(mp, fqs);
}

STATIC int
xfs_fs_get_xstatev(
	struct super_block	*sb,
	struct fs_quota_statv	*fqs)
{
	struct xfs_mount	*mp = XFS_M(sb);

	if (!XFS_IS_QUOTA_RUNNING(mp))
		return -ENOSYS;
	return xfs_qm_scall_getqstatv(mp, fqs);
}

static unsigned int
xfs_quota_flags(unsigned int uflags)
{
	unsigned int flags = 0;

	if (uflags & FS_QUOTA_UDQ_ACCT)
		flags |= XFS_UQUOTA_ACCT;
	if (uflags & FS_QUOTA_PDQ_ACCT)
		flags |= XFS_PQUOTA_ACCT;
	if (uflags & FS_QUOTA_GDQ_ACCT)
		flags |= XFS_GQUOTA_ACCT;
	if (uflags & FS_QUOTA_UDQ_ENFD)
		flags |= XFS_UQUOTA_ENFD;
	if (uflags & FS_QUOTA_GDQ_ENFD)
		flags |= XFS_GQUOTA_ENFD;
	if (uflags & FS_QUOTA_PDQ_ENFD)
		flags |= XFS_PQUOTA_ENFD;

	return flags;
}

STATIC int
xfs_quota_enable(
	struct super_block	*sb,
	unsigned int		uflags)
{
	struct xfs_mount	*mp = XFS_M(sb);

	if (sb->s_flags & MS_RDONLY)
		return -EROFS;
	if (!XFS_IS_QUOTA_RUNNING(mp))
		return -ENOSYS;

	return xfs_qm_scall_quotaon(mp, xfs_quota_flags(uflags));
}

STATIC int
xfs_quota_disable(
	struct super_block	*sb,
	unsigned int		uflags)
{
	struct xfs_mount	*mp = XFS_M(sb);

	if (sb->s_flags & MS_RDONLY)
		return -EROFS;
	if (!XFS_IS_QUOTA_RUNNING(mp))
		return -ENOSYS;
	if (!XFS_IS_QUOTA_ON(mp))
		return -EINVAL;

	return xfs_qm_scall_quotaoff(mp, xfs_quota_flags(uflags));
}

STATIC int
xfs_fs_rm_xquota(
	struct super_block	*sb,
	unsigned int		uflags)
{
	struct xfs_mount	*mp = XFS_M(sb);
	unsigned int		flags = 0;

	if (sb->s_flags & MS_RDONLY)
		return -EROFS;

	if (XFS_IS_QUOTA_ON(mp))
		return -EINVAL;

	if (uflags & FS_USER_QUOTA)
		flags |= XFS_DQ_USER;
	if (uflags & FS_GROUP_QUOTA)
		flags |= XFS_DQ_GROUP;
	if (uflags & FS_PROJ_QUOTA)
		flags |= XFS_DQ_PROJ;

	return xfs_qm_scall_trunc_qfiles(mp, flags);
}

STATIC int
xfs_fs_get_dqblk(
	struct super_block	*sb,
	struct kqid		qid,
	struct qc_dqblk		*qdq)
{
	struct xfs_mount	*mp = XFS_M(sb);

	if (!XFS_IS_QUOTA_RUNNING(mp))
		return -ENOSYS;
	if (!XFS_IS_QUOTA_ON(mp))
		return -ESRCH;

	return xfs_qm_scall_getquota(mp, from_kqid(&init_user_ns, qid),
				      xfs_quota_type(qid.type), qdq);
}

STATIC int
xfs_fs_set_dqblk(
	struct super_block	*sb,
	struct kqid		qid,
	struct qc_dqblk		*qdq)
{
	struct xfs_mount	*mp = XFS_M(sb);

	if (sb->s_flags & MS_RDONLY)
		return -EROFS;
	if (!XFS_IS_QUOTA_RUNNING(mp))
		return -ENOSYS;
	if (!XFS_IS_QUOTA_ON(mp))
		return -ESRCH;

	return xfs_qm_scall_setqlim(mp, from_kqid(&init_user_ns, qid),
				     xfs_quota_type(qid.type), qdq);
}

const struct quotactl_ops xfs_quotactl_operations = {
	.get_xstatev		= xfs_fs_get_xstatev,
	.get_xstate		= xfs_fs_get_xstate,
	.quota_enable		= xfs_quota_enable,
	.quota_disable		= xfs_quota_disable,
	.rm_xquota		= xfs_fs_rm_xquota,
	.get_dqblk		= xfs_fs_get_dqblk,
	.set_dqblk		= xfs_fs_set_dqblk,
};
