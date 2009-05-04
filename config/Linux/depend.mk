# ----------------------------------------------------------------
# Specific rules
# ----------------------------------------------------------------

$(BLTLIBSO):	$(BLTLIBOBJ) $(BLTGENOBJ)
	@$(CHKDIR)
	$(SOCMD) $(SOFLAGS) -o $@ $(CPPFLAGS) $(BLTLIBOBJ) $(BLTGENOBJ) $(LIBS)

$(BLTMODSO):	$(BLTLIBOBJ) $(BLTGENOBJ)
	@$(CHKDIR)
	$(SOCMD) $(SOFLAGS) -o $@ $(CPPFLAGS) $(BLTLIBOBJ) $(BLTGENOBJ) $(LIBS)

$(BLTPRGEXE):	$(BLTPRGOBJ) $(BLTGENOBJ)
	@$(CHKDIR)
	$(LDCMD) -o $@ $(CPPFLAGS) $(DEFS) $(LDFLAGS) $(INCS) $(BLTPRGOBJ) $(BLTGENOBJ) $(LIBS)

# ----------------------------------------------------------------
# Pattern rules
# ----------------------------------------------------------------

$(TRGDEP):	$(GENHDRSRC)

$(OBJDIR)/%.o:		%.cpp
	@$(CHKDIR)
	$(CPPCMD) -c $< -o $@ $(CPPFLAGS) $(DEFS) $(INCS)

# Generated C++ objects.
$(OBJDIR)/%.o:		$(GENDIR)/%.cpp
	@$(CHKDIR)
	$(CPPCMD) -c $< -o $@ $(CPPFLAGS) $(DEFS) $(INCS)

$(OBJTAIL)/%:		%.cpp
	@$(CHKDIR)
	$(LDCMD) $< -o $@ $(CPPFLAGS) $(DEFS) $(LDFLAGS) $(INCS) $(LIBS)

$(OBJDIR)/%.d:		%.cpp
	@$(CHKDIR)
	@echo "Updating dependencies for $<"
	@set -e; $(CPPCMD) -MM $< $(CPPFLAGS) $(DEFS) $(INCS) | \
	egrep -v $(DEPFLT) | \
	perl -p -e 's#(\S+.o)\s*:#$(@D)/$$1 $@: #g' > $@; \
	[ -s $@ ] || rm -f $@

$(OBJDIR)/%.d:		$(GENDIR)/%.cpp
	@$(CHKDIR)
	@echo "Updating dependencies for $<"
	@set -e; $(CPPCMD) -MM $< $(CPPFLAGS) $(DEFS) $(INCS) | \
	egrep -v $(DEPFLT) | \
	perl -p -e 's#(\S+.o)\s*:#$(@D)/$$1 $@: #g' > $@; \
	[ -s $@ ] || rm -f $@

$(OBJDIR)/%.dx:		%.cpp
	@$(CHKDIR)
	@echo "Updating dependencies for $<"
	@set -e; $(CPPCMD) -MM $< $(CPPFLAGS) $(DEFS) $(INCS) | \
	egrep -v $(DEPFLT) | \
	perl -p -e 's#(\S+).o\s*:#$(OBJDIR)/$$1$(EXEEXT) $@: #g' > $@; \
	[ -s $@ ] || rm -f $@

# Makes a symbolic link _MOD.so -> _MOD_d.so
$(LIBDIR)/%$(SOEXT):		$(LIBDIR)/%$(DBGSFX)$(SOEXT)
	@$(CHKDIR)
	(cd $(LIBDIR) && ln -s $*$(DBGSFX)$(SOEXT) $*$(SOEXT))

# Makes a symbolic link lib_MOD.so -> _MOD.so
ifdef MODSO
$(LIBDIR)/$(LIBMODSO)$(OPTSFX)$(SOEXT):	$(LIBDIR)/$(MODSO)$(DBGSFX)$(SOEXT)
	@$(CHKDIR)
	cd $(LIBDIR) && \
	ln -fs $(MODSO)$(DBGSFX)$(SOEXT) $(LIBMODSO)$(OPTSFX)$(SOEXT)
endif

$(BLTSRCENVSH):
	@$(CHKDIR)
	@echo "Constructing $@ ..."
	@echo "#!/bin/sh" > $@
	@echo "" >> $@
	@echo "export LD_LIBRARY_PATH=$(shell echo $(strip $(ENVLDLIBPATH)) | sed -e 's# #:#g')" >> $@
	@echo "" >> $@
	@echo "export PYTHONPATH=$(shell echo $(strip $(ENVPYTHONPATH)) | sed -e 's# #:#g')" >> $@
	@chmod +x $@

# Local Variables:
# mode: Makefile
# tab-width: 4
# End:
