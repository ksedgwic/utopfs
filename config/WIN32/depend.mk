# ----------------------------------------------------------------
# Specific rules
# ----------------------------------------------------------------

$(BLTLIBSO):	$(BLTLIBOBJ) $(BLTGENOBJ)
	@$(CHKDIR)
	$(SOCMD) -OUT:$@ -IMPLIB:$(@:%.dll=%.lib) $(SOFLAGS) $(LDFLAGS) $(BLTLIBOBJ) $(BLTGENOBJ) $(LIBS)
ifdef MANIFEST_EMBED
	$(SOCMT) $(SOCMTFLAGS) $(@:%.dll=%.dll.manifest) "$(SOCMTOUTPUTRESOURCE):$@;2"
endif

$(BLTMODSO):	$(BLTLIBOBJ) $(BLTGENOBJ)
	@$(CHKDIR)
	$(SOCMD) -OUT:$@ -IMPLIB:$(@:%.dll=%.lib) $(SOFLAGS) $(LDFLAGS) $(BLTLIBOBJ) $(BLTGENOBJ) $(LIBS)
ifdef MANIFEST_EMBED
	$(SOCMT) $(SOCMTFLAGS) $(@:%.dll=%.dll.manifest) "$(SOCMTOUTPUTRESOURCE):$@;2"
endif

$(BLTMODSOPYD):	$(BLTLIBOBJ) $(BLTGENOBJ)
	@$(CHKDIR)
	$(SOCMD) -OUT:$@ -IMPLIB:$(@:%.pyd=%.lib) $(SOFLAGS) $(LDFLAGS) $(BLTLIBOBJ) $(BLTGENOBJ) $(LIBS)
ifdef MANIFEST_EMBED
	$(SOCMT) $(SOCMTFLAGS) $(@:%.pyd=%.pyd.manifest) "$(SOCMTOUTPUTRESOURCE):$@;2"
endif

$(BLTPRGEXE):	$(BLTPRGOBJ) $(BLTGENOBJ)
	@$(CHKDIR)
	$(LDCMD) /OUT:$@ $(BLTPRGOBJ) $(BLTGENOBJ) $(LDFLAGS) $(LIBS)

# ----------------------------------------------------------------
# Pattern rules
# ----------------------------------------------------------------

$(TRGDEP):	$(GENHDRSRC)

$(OBJDIR)/%$(OBJEXT):		%.cpp
	@$(CHKDIR)
	$(CPPCMD) -c $< -Fo$@ $(CPPFLAGS) $(DEFS) $(INCS)

$(OBJDIR)/%$(OBJEXT):		$(GENDIR)/%.cpp
	@$(CHKDIR)
	$(CPPCMD) -c $< -Fo$@ $(CPPFLAGS) $(DEFS) $(INCS)

$(OBJTAIL)/%$(OBJEXT):		%.cpp
	@$(CHKDIR)
	$(CPPCMD) -c $< -Fo$@ $(CPPFLAGS) $(DEFS) $(INCS)

$(OBJTAIL)/%$(EXEEXT):		$(OBJTAIL)/%$(OBJEXT)
	@$(CHKDIR)
	$(LDCMD) /OUT:$@ $(LDFLAGS) $< $(LIBS)

LLP1 = $(strip $(ENVLDLIBPATH))

LLP2 = $(subst $(NULL) /,:/,$(LLP1)):\$${PATH}

LLP3 = $(subst $(NULL) ,\ ,$(LLP2))

ifdef ENVPYTHONPATH
PP1 = $(strip $(shell echo $(ENVPYTHONPATH) | \
	xargs -n 1 cygpath -w -a | \
	sed -e 's/\\/\\\\\\\\\\\\\\\\/g'))

PP2 = $(shell echo $(strip $(PP1)) | \
	sed -e 's/ /\\;/g')
endif

$(BLTSRCENVSH):
	@$(CHKDIR)
	@echo "Constructing $@ ..."
	@echo "#!/bin/sh" > $@
	@echo "" >> $@
	@echo "export PATH=$(LLP3)" >> $@
ifdef ENVPYTHONPATH
	@echo "" >> $@
	@echo "export PYTHONPATH=$(PP2)" >> $@
endif
	@chmod +x $@

# Local Variables:
# mode: Makefile
# tab-width: 4
# End:
