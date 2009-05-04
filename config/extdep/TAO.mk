ifndef TAO_mk__
       TAO_mk__ = 1

# ----------------------------------------------------------------
# Definitions
# ----------------------------------------------------------------

STUBEND = 		C
SKELEND = 		S

IDLTMPLOPTS	=	-GIe _impl -GIh _tmpl.h -GIs _tmpl.cpp
IDLFLAGS =		-in -GC $(IDLTMPLOPTS) $(IDLINCS)
IDLINCS +=		-I$(IDLDIR)

BLTSKELHDR = 	$(IDLSRV:%.idl=$(GENDIR)/%$(SKELEND).h)
BLTSKELINL =	$(IDLSRV:%.idl=$(GENDIR)/%$(SKELEND).inl)
BLTSKELSRC = 	$(IDLSRV:%.idl=$(GENDIR)/%$(SKELEND).cpp)
BLTSKELOBJ = 	$(BLTSKELSRC:$(GENDIR)/%.cpp=$(OBJDIR)/%$(OBJEXT))

# Server programs always need the stubs too ...
IDLCLIX =		$(IDLCLI) $(IDLSRV)

BLTSTUBHDR =	$(IDLCLIX:%.idl=$(GENDIR)/%$(STUBEND).h)
BLTSTUBINL =	$(IDLCLIX:%.idl=$(GENDIR)/%$(STUBEND).inl)
BLTSTUBSRC =	$(IDLCLIX:%.idl=$(GENDIR)/%$(STUBEND).cpp)
BLTSTUBOBJ = 	$(BLTSTUBSRC:$(GENDIR)/%.cpp=$(OBJDIR)/%$(OBJEXT))

BLTIDLSRC =		$(BLTSKELSRC) $(BLTSTUBSRC)

# Append to the global build lists below.

GENHDRSRC +=	$(BLTSKELSRC) $(BLTSKELHDR) $(BLTSKELINL) \
				$(BLTSTUBSRC) $(BLTSTUBHDR) $(BLTSTUBINL)

BLTDEP +=		$(BLTIDLSRC:$(GENDIR)/%.cpp=$(OBJDIR)/%.dx)

BLTGENOBJ +=	$(BLTSKELOBJ) $(BLTSTUBOBJ)

ifeq ($(SYSNAME),Linux)
# On Linux we assume that TAO is installed in the standard places via
# "tao" and "tao-devel" packages.
IDLCMD =		tao_idl

IDLFLAGS +=		-Wb,export_macro=$(EXPMACRO),export_include=$(EXPHEADER)

LIBS +=			\
				-lTAO_BiDirGIOP \
				-lTAO_Messaging \
				-lTAO_IORTable \
				-lTAO_PortableGroup \
				-lTAO \
				$(NULL)
endif

ifeq ($(SYSNAME),WIN32)
IDLCMD =		$(ACEROOT)/bin/tao_idl

IDLFLAGS +=		-Wb,export_macro=$(EXPMACRO),export_include=$(EXPHEADER)

INCS +=			-I$(ACEROOT)/TAO

LIBS +=			\
				$(ACEROOT)/lib/TAO$(LIBSFX3).lib \
				$(ACEROOT)/lib/TAO_BiDirGIOP$(LIBSFX3).lib \
				$(ACEROOT)/lib/TAO_AnyTypeCode$(LIBSFX3).lib \
				$(ACEROOT)/lib/TAO_IORTable$(LIBSFX3).lib \
				$(ACEROOT)/lib/TAO_Messaging$(LIBSFX3).lib \
				$(ACEROOT)/lib/TAO_ValueType$(LIBSFX3).lib \
				$(ACEROOT)/lib/TAO_PortableGroup$(LIBSFX3).lib \
				$(ACEROOT)/lib/TAO_PortableServer$(LIBSFX3).lib \
				$(NULL)
endif

# ----------------------------------------------------------------
# Dependencies 
# ----------------------------------------------------------------

$(BLTLIBSO):	$(BLTGENOBJ)

$(BLTMODSO):	$(BLTGENOBJ)

$(BLTPRGEXE):	$(BLTGENOBJ)

$(GENDIR)/%$(STUBEND).h $(GENDIR)/%$(STUBEND).inl $(GENDIR)/%$(STUBEND).cpp:	$(IDLDIR)/%.idl
	@$(CHKDIR)
	$(IDLCMD) -o $(@D) -SS $(IDLFLAGS) $<

$(GENDIR)/%$(SKELEND).h $(GENDIR)/%$(SKELEND).inl $(GENDIR)/%$(SKELEND).cpp \
$(GENDIR)/%$(STUBEND).h $(GENDIR)/%$(STUBEND).inl $(GENDIR)/%$(STUBEND).cpp:	$(IDLDIR)/%.idl
	@$(CHKDIR)
	$(IDLCMD) -o $(@D) $(IDLFLAGS) $<

# Keep the generated files around.
.PRECIOUS:		$(GENHDRSRC)

# Always generate source before dependencies.
$(TRGDEP):		$(GENHDRSRC)

ifeq ($(SYSNAME),Linux)
all::			$(BLTDEP)
endif

INIT::			$(GENHDRSRC)

include $(ROOTDIR)/config/extdep/ACE.mk

# Local Variables:
# mode: Makefile
# tab-width: 4
# End:

endif # TAO_mk__
