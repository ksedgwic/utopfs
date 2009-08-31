ifndef openssl_mk__
       openssl_mk__ = 1

ifeq ($(SYSNAME),Linux)
INCS +=		$(shell pkg-config --cflags openssl)
LIBS +=		$(shell pkg-config --libs openssl)
endif

ifeq (,$(filter-out WIN32, $(SYSNAME)))
OPENSSLROOT =	$(shell cygpath -m $(ABSDIST))

INCS +=	-I$(OPENSSLROOT)/include

LIBS +=	$(OPENSSLROOT)/lib/libeay32.lib \
		$(OPENSSLROOT)/lib/ssleay32.lib	\
		$(NULL)

# Need OpenSSL stuff here.
endif

# Local Variables:
# mode: Makefile
# tab-width: 4
# End:

endif # openssl_mk__
