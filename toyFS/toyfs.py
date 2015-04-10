#!/usr/bin/env python

import os
import logging
import fuse
import time

import toy

fuse.fuse_python_api = (0, 2)

# Logging
logger    = logging.getLogger("toyfs")
handler   = logging.FileHandler('toyfs.log')
logger.addHandler(handler)
logger.setLevel(logging.INFO)

fs = toy.Toy()

# toyFS
class ToyFS(fuse.Fuse):
    def getattr(self, path):
        logger.info("[getattr] (path=%s)" % path)
        st = fs.getattr(path)
        return st

    def readdir(self, path, offset):            
        logger.info("[readdir] (path=%s, offset=%s)" % (path, offset))
        directories = fs.readdir(path)
        for directory in directories:
            yield fuse.Direntry(directory)

    def open(self, path, flags):
        logger.info("[open] (path=%s, flags=%s)" % (path, flags))

    def read(self, path, length, offset):
        logger.info("[read] (path=%s, length=%s, offset=%s)" % (path, length, offset))
        return fs.read(path, length, offset)
    
if __name__ == '__main__':
    toyfs = ToyFS(version = '%prog ' + '0.0.1',
               usage = 'Toy filesystem ' + fuse.Fuse.fusage,
               dash_s_do = 'setsingle')
    toyfs.parse(errex = 1)
    toyfs.flags = 0
    toyfs.multithreaded = 0
    toyfs.main()
