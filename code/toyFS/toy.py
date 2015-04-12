import fuse
import stat
import errno
import time

class ToyStat(fuse.Stat):
    def __init__(self):
        """
        The following stat structure members are implemented.
        """
        self.st_mode  = 0      # (protection bits)
        self.st_ino   = 0      # (inode number)
        self.st_dev   = 0      # (device)
        self.st_nlink = 0      # (number of hard links)
        self.st_uid   = 500    # (user ID of owner)
        self.st_gid   = 500    # (group ID of owner) 
        self.st_size  = 0      # (size of file, in bytes)
        self.st_atime = 0      # (time of most recent access)
        self.st_mtime = 0      # (time of most recent content modification)
        self.st_ctime = 0      # (time of most recent metadata change)

# Helper for Toy filesystem
class Toy():
    def __init__(self):
        self.data = 'Lorem ipsum dolor sit amet, consectetur adipiscing elit. In augue sapien, congue fermentum tortor ut, consequat maximus posuere.\n'
        
    def getattr(self, path):
        st = ToyStat()
        if path.endswith('.txt'):
            st.st_mode = stat.S_IFREG | 0644
            st.st_size = len(self.data)
        else:
            st.st_mode = stat.S_IFDIR | 0755
        return st

    def readdir(self, path):
        contents = {
            '/':           ['hello', 'world', 'docs'],
            '/hello':      ['welt'],
            '/hello/welt': ['last_level'],
            '/docs':       ['readme.txt']
        }
        res = ['.','..'] + contents.get(path)
        return res

    def read(self, path, length, offset):
        return self.data
    
