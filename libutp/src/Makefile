ROOTDIR = ../..

include $(ROOTDIR)/config/define.mk

PROTO +=	\
			Assembly.proto \
			HeadEdge.proto \
			Stats.proto \
			$(NULL)

LIBSRC =	\
			Assembly.cpp \
			Base32.cpp \
			Base64.cpp \
			BlockCipher.cpp \
			BlockStore.cpp \
			BlockStoreFactory.cpp \
			Digest.cpp \
			Except.cpp \
			FileSystem.cpp \
			FileSystemFactory.cpp \
			FormatterBase.cpp \
			LameHeadNodeGraph.cpp \
			Log.cpp \
			LogField.cpp \
			LoggerBase.cpp \
			MD5.cpp \
			Random.cpp \
			Stats.cpp \
			StreamCipher.cpp \
			ThreadPool.cpp \
			T64.cpp \
			utplog.cpp \
			$(NULL)

LIBSO =		libUTPFS-utp

EXPMACRO =	UTP_EXP
EXPHEADER =	utpexp.h
DEFS +=		-DUTP_IMPL

include $(ROOTDIR)/config/depend.mk

# External Dependencies
include $(ROOTDIR)/config/extdep/protobuf.mk
include $(ROOTDIR)/config/extdep/ACE.mk
include $(ROOTDIR)/config/extdep/openssl.mk

# Local Variables:
# mode: Makefile
# tab-width: 4
# End:
