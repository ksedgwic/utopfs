ifndef protobuf_mk__
       protobuf_mk__ = 1

BLTPBHDR = $(PROTO:%.proto=$(GENDIR)/%.pb.h)
BLTPBSRC = $(PROTO:%.proto=$(GENDIR)/%.pb.cpp)
BLTPBOBJ = $(BLTPBSRC:$(GENDIR)/%.cpp=$(OBJDIR)/%$(OBJEXT))

BLTPBGEN = $(BLTPBHDR) $(BLTPBSRC)

BLTLIBOBJ += $(BLTPBOBJ)

GENHDRSRC +=	$(BLTPBHDR) $(BLTPBSRC)

BLTDEP +=		$(BLTPBSRC:$(GENDIR)/%.cpp=$(OBJDIR)/%.dx)

ifneq (,$(BLTPBGEN))
CLEANFILES += $(BLTPBGEN)
CLOBBERFILES += $(BLTPBGEN)
endif

ifeq (,$(filter-out Linux, $(SYSNAME)))
PROTOC = /usr/bin/protoc
LIBS += -lprotobuf
endif

ifeq (,$(filter-out WIN32, $(SYSNAME)))
PROTOC = $(ABSDIST)/bin/protoc
ifdef BUILD_OPT
DEFS += -DPROTOBUF_USE_DLLS
endif
LIBS += $(DIST)/lib/libprotobuf.lib
endif

ifneq (,$(BLTPBSRC))
CXXSRCS += $(BLTPBSRC)
endif

ifneq (,$(BLTPBHDR))
EXPORTS += $(BLTPBHDR)
endif

# ----------------------------------------------------------------

$(GENDIR)/%.pb.h $(GENDIR)/%.pb.cpp:	%.proto
	@$(CHKDIR)
	sh -c "($(PROTOC) --cpp_out=dllexport_decl=$(EXPMACRO):$(GENDIR) $< && \
		mv $(GENDIR)/$*.pb.cc $(GENDIR)/$*.pb.cpp && \
		sed -i 's|#include <string>|#include \"$(EXPHEADER)\"\n&|' $(GENDIR)/$*.pb.h)"

clean::
	rm -f $(BLTPBGEN)

clobber::	clean

$(BLTLIBSO):	$(BLTPBOBJ)

$(BLTMODSO):	$(BLTPBOBJ)

$(BLTLIBLIB):	$(BLTPBOBJ)

.PRECIOUS:	$(GENHDRSRC)

# Always generate source before dependencies.
$(TRGDEP):	$(GENHDRSRC)

INIT::		$(GENHDRSRC)

## ifeq ($(SYSNAME),Linux)
## all::		$(BLTDEP)
## endif

# Local Variables:
# mode: Makefile
# tab-width: 4
# End:

endif # protobuf_mk__
