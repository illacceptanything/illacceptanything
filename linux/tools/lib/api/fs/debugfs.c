#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/vfs.h>
#include <sys/mount.h>
#include <linux/kernel.h>

#include "debugfs.h"

char debugfs_mountpoint[PATH_MAX + 1] = "/sys/kernel/debug";

static const char * const debugfs_known_mountpoints[] = {
	"/sys/kernel/debug",
	"/debug",
	0,
};

static bool debugfs_found;

/* find the path to the mounted debugfs */
const char *debugfs_find_mountpoint(void)
{
	const char * const *ptr;
	char type[100];
	FILE *fp;

	if (debugfs_found)
		return (const char *)debugfs_mountpoint;

	ptr = debugfs_known_mountpoints;
	while (*ptr) {
		if (debugfs_valid_mountpoint(*ptr) == 0) {
			debugfs_found = true;
			strcpy(debugfs_mountpoint, *ptr);
			return debugfs_mountpoint;
		}
		ptr++;
	}

	/* give up and parse /proc/mounts */
	fp = fopen("/proc/mounts", "r");
	if (fp == NULL)
		return NULL;

	while (fscanf(fp, "%*s %" STR(PATH_MAX) "s %99s %*s %*d %*d\n",
		      debugfs_mountpoint, type) == 2) {
		if (strcmp(type, "debugfs") == 0)
			break;
	}
	fclose(fp);

	if (strcmp(type, "debugfs") != 0)
		return NULL;

	debugfs_found = true;

	return debugfs_mountpoint;
}

/* verify that a mountpoint is actually a debugfs instance */

int debugfs_valid_mountpoint(const char *debugfs)
{
	struct statfs st_fs;

	if (statfs(debugfs, &st_fs) < 0)
		return -ENOENT;
	else if ((long)st_fs.f_type != (long)DEBUGFS_MAGIC)
		return -ENOENT;

	return 0;
}

/* mount the debugfs somewhere if it's not mounted */
char *debugfs_mount(const char *mountpoint)
{
	/* see if it's already mounted */
	if (debugfs_find_mountpoint())
		goto out;

	/* if not mounted and no argument */
	if (mountpoint == NULL) {
		/* see if environment variable set */
		mountpoint = getenv(PERF_DEBUGFS_ENVIRONMENT);
		/* if no environment variable, use default */
		if (mountpoint == NULL)
			mountpoint = "/sys/kernel/debug";
	}

	if (mount(NULL, mountpoint, "debugfs", 0, NULL) < 0)
		return NULL;

	/* save the mountpoint */
	debugfs_found = true;
	strncpy(debugfs_mountpoint, mountpoint, sizeof(debugfs_mountpoint));
out:
	return debugfs_mountpoint;
}

int debugfs__strerror_open(int err, char *buf, size_t size, const char *filename)
{
	char sbuf[128];

	switch (err) {
	case ENOENT:
		if (debugfs_found) {
			snprintf(buf, size,
				 "Error:\tFile %s/%s not found.\n"
				 "Hint:\tPerhaps this kernel misses some CONFIG_ setting to enable this feature?.\n",
				 debugfs_mountpoint, filename);
			break;
		}
		snprintf(buf, size, "%s",
			 "Error:\tUnable to find debugfs\n"
			 "Hint:\tWas your kernel compiled with debugfs support?\n"
			 "Hint:\tIs the debugfs filesystem mounted?\n"
			 "Hint:\tTry 'sudo mount -t debugfs nodev /sys/kernel/debug'");
		break;
	case EACCES:
		snprintf(buf, size,
			 "Error:\tNo permissions to read %s/%s\n"
			 "Hint:\tTry 'sudo mount -o remount,mode=755 %s'\n",
			 debugfs_mountpoint, filename, debugfs_mountpoint);
		break;
	default:
		snprintf(buf, size, "%s", strerror_r(err, sbuf, sizeof(sbuf)));
		break;
	}

	return 0;
}

int debugfs__strerror_open_tp(int err, char *buf, size_t size, const char *sys, const char *name)
{
	char path[PATH_MAX];

	snprintf(path, PATH_MAX, "tracing/events/%s/%s", sys, name ?: "*");

	return debugfs__strerror_open(err, buf, size, path);
}
