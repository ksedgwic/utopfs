import _PyDirEntryFunc

class PyDirEntryFunc:
    def __init__(self):
        self._base = _PyDirEntryFunc.new(self)
        
    def def_entry(self, name, stat, offset):
        print "def_entry", name, stat, offset
