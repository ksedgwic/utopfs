# ----------------------------------------------------------------
# Basic Targets
# ----------------------------------------------------------------

all::		INIT $(ALLTRG)
		+$(LOOP_SUBDIRS)

rpm::		$(ALLTRG)
		+$(LOOP_SUBDIRS)

install::	$(INSTTRG)
		+$(LOOP_SUBDIRS)

test::
		+$(LOOP_SUBDIRS)

echo::
		+$(LOOP_SUBDIRS)

test::	$(BLTTESTSUITE)

clean::
ifdef CLEANFILES
		rm -f $(CLEANFILES)
endif
ifdef CLEANDIRS
		rm -rf $(CLEANDIRS)
endif
		+$(LOOP_SUBDIRS)

clobber::
ifdef CLOBBERFILES
		rm -f $(CLOBBERFILES)
endif
ifdef CLOBBERDIRS
		rm -rf $(CLOBBERDIRS)
endif
		+$(LOOP_SUBDIRS)

# Dummy target which forces early production.
INIT::

FORCE:

.PRECIOUS:	$(GENHDRSRC)

# ----------------------------------------------------------------
# Dependencies
# ----------------------------------------------------------------

.PRECIOUS:	$(TRGDEP) $(TRGDEPX)

INIT::	$(TRGDEP) $(TRGDEPX)

# include dependencies if we are not cleaning or clobbering
ifneq ($(findstring clean,$(MAKECMDGOALS)),clean)
ifneq ($(findstring clobber,$(MAKECMDGOALS)),clobber)
-include $(TRGDEP) $(TRGDEPX)
endif
endif


# ----------------------------------------------------------------
# Directory Stuff
# ----------------------------------------------------------------

# macro to construct needed target directories
define CHKDIR
if test ! -d $(@D); then $(INSTDIRCMD) $(@D); else true; fi
endef

# macro which recurses into SUBDIRS
ifdef SUBDIRS
LOOP_SUBDIRS = \
	@for d in $(SUBDIRS); do \
		set -e; \
		echo "cd $$d; $(MAKE) $@"; \
		$(MAKE) -C $$d $@; \
		set +e; \
	done
endif


# ----------------------------------------------------------------
# General Dependencies
# ----------------------------------------------------------------

$(BLTLIBSO):	$(BLTLIBOBJ)

$(BLTMODSO):	$(BLTLIBOBJ)

$(BLTPRGEXE):	$(BLTPRGOBJ)


# ----------------------------------------------------------------
# Specific Rules
# ----------------------------------------------------------------

$(BLTTESTSUITE):
		sh -c ". $(SRCENVSH) && $(PYTHONCMD) $(RUNSUITE)"

$(OBJDIR)/%:	%
		@$(CHKDIR)
		$(INSTFILECMD) $< $@

$(INSTLIBSO):	$(BLTLIBSO)
		@$(CHKDIR)
		$(INSTFILECMD) $< $@

$(INSTMODSO):	$(BLTMODSO)
		@$(CHKDIR)
		$(INSTFILECMD) $< $@

$(INSTPRGEXE):	$(BLTPRGEXE)
		@$(CHKDIR)
		$(INSTFILECMD) $< $@

# ----------------------------------------------------------------
# Configuration Targets
# ----------------------------------------------------------------

$(INSTCFGDIR)/%:		% $(INSTALLCFG)
	@$(CHKDIR)
	sh -c "m4 $(M4DEFS) $(INSTALLCFG) $(M4S) $< > $@"
	@if test ! -z $(findstring $@, $(BLTSCRIPTS)); then \
		echo chmod +x $@; \
		chmod +x $@; \
	fi

$(WORKCFGDIR)/%:		% $(WORKINGCFG)
	@$(CHKDIR)
	sh -c "m4 $(M4DEFS) $(WORKINGCFG) $(M4S) $< > $@"
	@if test ! -z $(findstring $@, $(BLTSCRIPTS)); then \
		echo chmod +x $@; \
		chmod +x $@; \
	fi

include $(ROOTDIR)/config/$(SYSNAME)/depend.mk
