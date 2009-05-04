ifndef python_mk__
       python_mk__ = 1

ifeq ($(SYSNAME),Linux)
INCS +=		$(shell python-config --includes)
LIBS +=		$(shell python-config --ldflags)
endif

ifeq (,$(filter-out WIN32, $(SYSNAME)))
PYTROOT =	c:/Python-2.5
INCS +=		-I$(PYTROOT)/include -I$(PYTROOT)/PC
ifeq ($(BUILD),DEBUG)
LIBS +=		$(PYTROOT)/PCbuild/python25_d.lib
else
LIBS +=		$(PYTROOT)/PCbuild/python25.lib
endif
ENVLDLIBPATH +=	$(shell cygpath ${PYTROOT}/PCbuild)
endif

# Local Variables:
# mode: Makefile
# tab-width: 4
# End:

endif # python_mk__
