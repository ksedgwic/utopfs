INCS +=		\
			-I$(ROOTDIR)/libutp/src \
			-I$(ROOTDIR)/libutp/src/$(GENTAIL) \
			$(NULL)

ENVLDLIBPATH +=	\
			$(ABSROOT)/libutp/src/$(OBJTAIL) \
			$(NULL)

ifeq ($(SYSNAME), Linux)
LIBS +=		\
			-L$(ROOTDIR)/libutp/src/$(OBJTAIL) -lutp \
			$(NULL)
endif

ifeq ($(SYSNAME), WIN32)
LIBS +=		\
			$(ROOTDIR)/libutp/src/$(OBJTAIL)/libutp.lib \
			$(NULL)
endif

# Local Variables:
# mode: Makefile
# tab-width: 4
# End:
