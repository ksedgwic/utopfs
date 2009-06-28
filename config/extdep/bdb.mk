ifndef bdb_mk__
       bdb_mk__ = 1

ifeq ($(SYSNAME),Linux)

LIBS +=         -ldb_cxx

endif

# Local Variables:
# mode: Makefile
# tab-width: 4
# End:

endif # bdb_mk__
