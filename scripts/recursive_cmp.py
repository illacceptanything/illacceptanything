#!/usr/bin/env python3

CHECK_CONTENT = False

import os
import sys
import subprocess

join = os.path.join

def compare_dirs(dir1, dir2):
    l1 = set(os.listdir(dir1))
    l2 = set(os.listdir(dir2))
    if l1 != l2:
        print('“%s” and “%s” differ:' % (dir1, dir2))
        print('\tfirst has: %s' % (l1-l2))
        print('\tsecond has: %s' % (l2-l1))
    for filename in (l1 & l2):
        compare(join(dir1, filename), join(dir2, filename))

def compare_files(f1, f2):
    if CHECK_CONTENT:
        subprocess.call(['cmp', f1, f2])

def compare(dir1, dir2):
    if os.path.isdir(dir1) and os.path.isdir(dir2):
        return compare_dirs(dir1, dir2)
    elif os.path.isfile(dir1) and os.path.isfile(dir2):
        return compare_files(dir1, dir2)
    elif not os.path.isdir(dir1) and not os.path.isfile(dir1) and \
            not os.path.isdir(dir2) and not os.path.isfile(dir2):
        print('Skipping “%s” and “%s”: neither file or dir.' % (dir1, dir2))
    else:
        print('“%s” and “%s” are not the same type (dir/file).' % (dir1, dir2))

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print('Syntax: %s dir1 dir2' % sys.argv[0])
        exit(1)

    (prog, dir1, dir2) = sys.argv
    compare(dir1, dir2)
