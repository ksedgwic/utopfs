DOXCMD =	doxygen

dox:
		(cd $(ROOTDIR) && $(DOXCMD) config/Doxyfile)
		(cd $(ROOTDIR)/doc/latex && make refman.pdf)
