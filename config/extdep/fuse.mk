ifndef fuse_mk__
       fuse_mk__ = 1

ifeq ($(SYSNAME),Linux)

DEFS +=         -D_FILE_OFFSET_BITS=64
LIBS +=         -lfuse

endif

# Local Variables:
# mode: Makefile
# tab-width: 4
# End:

endif # fuse_mk__
