INCS +=		\
			-I$(ROOTDIR)/libutp/pymod \
			$(NULL)

ENVLDLIBPATH +=	\
			$(ABSROOT)/libutp/pymod/$(OBJTAIL) \
			$(NULL)

ifeq ($(SYSNAME), Linux)
LIBS +=		\
			-L$(ROOTDIR)/libutp/pymod/$(OBJTAIL) -l_utp \
			$(NULL)
endif

ifeq ($(SYSNAME), WIN32)
LIBS +=		\
			$(ROOTDIR)/libutp/pymod/$(OBJTAIL)/lib_utp.lib \
			$(NULL)
endif

ENVPYTHONPATH += \
			$(ABSROOT)/libutp/pylib \
			$(ABSROOT)/libutp/pymod/$(OBJTAIL) \
			$(NULL)

# Local Variables:
# mode: Makefile
# tab-width: 4
# End:
