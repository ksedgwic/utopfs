ifndef ruby_mk__
       ruby_mk__ = 1

ifeq ($(SYSNAME),Linux)

RUBYARCHDIR=`ruby -rrbconfig -e 'puts Config::CONFIG["archdir"]'`

INCS +=			-I$(RUBYARCHDIR)

endif

# Local Variables:
# mode: Makefile
# tab-width: 4
# End:

endif # ruby_mk__
