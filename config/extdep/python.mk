ifndef python_mk__
       python_mk__ = 1

ifeq ($(SYSNAME),Linux)
INCS +=		$(shell python-config --includes)
LIBS +=		$(shell python-config --ldflags)
endif

ifeq (,$(filter-out WIN32, $(SYSNAME)))
PYTROOT =	$(shell cygpath -m $(ABSDIST))
INCS +=		-I$(PYTROOT)/include/python
ifeq ($(BUILD),DEBUG)
LIBS +=		$(PYTROOT)/lib/python25_d.lib
else
LIBS +=		$(PYTROOT)/lib/python25.lib
endif
ENVLDLIBPATH +=	$(shell cygpath ${PYTROOT})
endif

# Local Variables:
# mode: Makefile
# tab-width: 4
# End:

endif # python_mk__
