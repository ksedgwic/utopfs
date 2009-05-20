ROOTDIR = ../..

include $(ROOTDIR)/config/define.mk

LIBSRC =	\
			Base32.cpp \
			BlockStore.cpp \
			Digest.cpp \
			Except.cpp \
			FileSystem.cpp \
			FormatterBase.cpp \
			Log.cpp \
			LogField.cpp \
			LoggerBase.cpp \
			T64.cpp \
			$(NULL)

LIBSO =		libutp

EXPMACRO =	UTP_EXP
EXPHEADER =	utpexp.h
DEFS +=		-DUTP_IMPL

include $(ROOTDIR)/config/depend.mk

# External Dependencies
include $(ROOTDIR)/config/extdep/openssl.mk

# Local Variables:
# mode: Makefile
# tab-width: 4
# End:
