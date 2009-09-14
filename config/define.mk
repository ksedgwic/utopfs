SYSNAME = $(subst /,_,$(shell uname -s))

ifeq (,$(filter-out WINNT CYGWIN_NT-5.0 CYGWIN_NT-5.1, $(SYSNAME)))
SYSNAME = WIN32
endif

# If neither is defined, default to DEBUG
ifndef BUILD
BUILD=DEBUG
endif

ifeq ($(BUILD),RELEASE)
OBJTAIL =		$(SYSNAME).OPTOBJ
OPTSFX =
DBGSFX =
else
OBJTAIL =		$(SYSNAME).DBGOBJ
OPTSFX =
DBGSFX =
endif

GENTAIL =		GENSRC

GENDIR =		$(GENTAIL)
OBJDIR =		$(OBJTAIL)
LIBDIR =		$(OBJTAIL)
BINDIR =		$(OBJTAIL)
INSTCFGDIR =	$(SYSNAME).INSTCFG
WORKCFGDIR =	$(SYSNAME).WORKCFG
CFGDIR =		$(INSTCFGDIR) $(WORKCFGDIR)

INSTLIBDIR = 	$(INSTPFX)/lib
INSTBINDIR =	$(INSTPFX)/bin
INSTPYLIBDIR =	$(INSTPFX)/lib/python

MEMFILES =		*.mem
SEGFILES =		*.seg
DBFILES =		*.db
LOGFILES =		*.log
COREFILES =		core.* core
PYCFILES =		*.pyc
TILDEFILES =	*~

# ----------------------------------------------------------------
# Built Components, in the source tree
# ----------------------------------------------------------------

## Why did we do this?  Undoing for now ...
## LIBMODSO =		$(patsubst lib_%,lib%,lib$(MODSO))
LIBMODSO =		$(MODSO:%=lib%)

BLTLIBOBJ = 	$(LIBSRC:%.cpp=$(OBJDIR)/%$(OBJEXT))
BLTPRGOBJ =		$(PRGSRC:%.cpp=$(OBJDIR)/%$(OBJEXT))
BLTLIBSO =		$(LIBSO:%=$(LIBDIR)/%$(SOEXT))
BLTMODSO =		$(MODSO:%=$(LIBDIR)/%$(DBGSFX)$(SOEXT))
BLTMODSOPYD =	$(MODSOPYD:%=$(LIBDIR)/%$(DBGSFX)$(PYDEXT))
BLTLIBMODOPTSO = $(MODSO:%=$(LIBDIR)/$(LIBMODSO)$(OPTSFX)$(SOEXT))
BLTTSTEXE =		$(TSTSRC:%.cpp=$(OBJTAIL)/%$(EXEEXT))
BLTPRGEXE =		$(PRGEXE:%=$(BINDIR)/%$(DBGSFX)$(EXEEXT))
BLTSRCENVSH0 =	$(sort $(BLTTSTEXE:%=$(OBJTAIL)/$(SRCENVSH)))
BLTSRCENVSH1 =	$(BLTPRGEXE:%=$(BINDIR)/$(SRCENVSH))
BLTSRCENVSH2 =	$(sort $(BLTSCRIPTS:%=$(WORKCFGDIR)/$(SRCENVSH)))
BLTSRCENVSH =	$(BLTSRCENVSH0) $(BLTSRCENVSH1) $(BLTSRCENVSH2)
BLTTESTSUITE =	$(sort $(TESTSUITE:%.py=RUNSUITE))

BLTCFG =		$(foreach cdir,$(CFGDIR),$(patsubst %,$(cdir)/%,$(CFG)))
BLTSCRIPTS =	$(foreach cdir,$(CFGDIR),$(patsubst %,$(cdir)/%,$(SCRIPTS)))

# ----------------------------------------------------------------
# Installed Components, in the install tree
# ----------------------------------------------------------------

INSTLIBSO =	$(BLTLIBSO:$(LIBDIR)/%=$(INSTLIBDIR)/%)
INSTMODSO =	$(BLTMODSO:$(LIBDIR)/%=$(INSTLIBDIR)/%)

INSTMODSOPYD =	$(BLTMODSOPYD:$(LIBDIR)/%=$(INSTLIBDIR)/%)

INSTPRGEXE =	$(BLTPRGEXE:$(BINDIR)/%=$(INSTBINDIR)/%)

INSTPYLIB =	$(PYLIB:%=$(INSTPYLIBDIR)/%)

# ----------------------------------------------------------------
# Dependencies
# ----------------------------------------------------------------

DEPSRC =	$(PRGSRC) $(LIBSRC) $(PYMODSRC)
DEPXSRC =	$(TSTSRC)

BLTDEP0 =	$(DEPSRC:$(GENDIR)/%.cpp=$(OBJDIR)/%.d)
BLTDEP =	$(BLTDEP0:%.cpp=$(OBJDIR)/%.d)
BLTDEPX =	$(DEPXSRC:%.cpp=$(OBJDIR)/%.dx)

# Platforms which support dependencies will:
# TRGDEP = BLTDEP
# TRGDEPX = BLTDEPX

# ----------------------------------------------------------------
# Lists of Finished Products
# ----------------------------------------------------------------

CLEANFILES =	$(GENHDRSRC) \
				$(BLTLIBSO) $(BLTMODSO) $(BLTMODSOPYD) $(BLTMODOPTSO) $(BLTLIBOBJ) \
				$(BLTTSTEXE) $(BLTPRGEXE) \
				$(MEMFILES) $(SEGFILES) $(DBFILES) $(LOGFILES) \
				$(COREFILES) $(PYCFILES) $(TILDEFILES)

CLOBBERFILES =	$(TRGDEP) $(TRGDEPX) $(BLTCFG) $(BLTSRCENVSH)

CLOBBERDIRS =	$(OBJDIR) $(GENDIR) $(LIBDIR) $(BINDIR) $(OBJTAIL) $(CFGDIR)

ALLTRG = 		$(GENHDRSRC) $(TRGDEP) $(TRGDEPX) \
				$(BLTLIBSO) $(BLTMODSO) $(BLTMODSOPYD) $(BLTMODOPTSO) \
				$(BLTTSTEXE) $(BLTPRGEXE) \
				$(BLTSRCENVSH) \
				$(BLTCFG) $(BLTSCRIPTS)

INSTTRG =		$(INSTLIBSO) $(INSTMODSO) $(INSTPRGEXE)

ifeq ($(SYSNAME), Linux)
ALLTRG +=		$(BLTLIBMODOPTSO)
endif

# ----------------------------------------------------------------
# Config processing
# ----------------------------------------------------------------

INSTALLCFG =		$(ROOTDIR)/config/$(SYSNAME)/sys.m4 \
			$(ROOTDIR)/config/install.m4

WORKINGCFG =		$(ROOTDIR)/config/$(SYSNAME)/sys.m4 \
			$(ROOTDIR)/config/working.m4

M4DEFS += 	\
		-DOBJDIR='$(OBJDIR)' \
		-DABSROOT='$(ABSROOT)' \
		-DCURRDIRW='$(CURRDIRW)' \
		-DCURRDIRw='$(CURRDIRw)' \
		-DCURRDIRU='$(CURRDIRU)' \
		-DPYTROOT='$(PYTROOT)' \
		$(NULL)

include $(ROOTDIR)/config/$(SYSNAME)/define.mk

ifdef MYOVERRIDES
include $(MYOVERRIDES)
endif

# Local Variables:
# mode: Makefile
# tab-width: 4
# End:
