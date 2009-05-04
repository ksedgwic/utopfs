ABSROOT =		$(shell cd $(ROOTDIR) && /usr/bin/pwd)

CURRDIRU =	$(shell pwd)
CURRDIRW =	$(subst \,\\,$(shell cygpath -w $(shell pwd)))
CURRDIRw =	$(wordlist 2, 2, $(subst :, ,$(CURRDIRW)))

OBJEXT =		.obj
SOEXT =			.dll
EXEEXT =		.exe

CPPCMD =		cl

ifeq ($(BUILD),DEBUG)
CPPFLAGS =		-Od -Z7 -MDd -W3 -nologo -GF -GR -GT -G5 -GX
else
CPPFLAGS =		-O2 -Ot -Z7 -MD -W3 -nologo -GF -GR -GT -G7 -GX -arch:SSE2 
endif

#RMFILESCMD = del /Q
RMDIRSCMD   = 	rmdir
RMDIRSFLAGS = 	/S /Q

ifdef VC71_ROOT
LDCMD =			"$(shell cygpath -m $(VC71_ROOT))/bin/link"
else
LDCMD =			link
endif

ifeq ($(BUILD),DEBUG)
LDFLAGS =		-nologo -DEBUG -DEBUGTYPE:CV -map
else
LDFLAGS =		-nologo -map
endif

ifdef VC71_ROOT
SOCMD =			"$(shell cygpath -m $(VC71_ROOT))/bin/link"
else
SOCMD =			link
endif


ifeq ($(BUILD),DEBUG)
SOFLAGS =		-nologo -DLL -SUBSYSTEM:WINDOWS -DEBUG \
				-DEBUGTYPE:CV -MAP
else
SOFLAGS =		-nologo -DLL -SUBSYSTEM:WINDOWS -INCREMENTAL:NO -LARGEADDRESSAWARE -TSAWARE:NO -OPT:REF -OPT:ICF /OPT:NOWIN98 -MACHINE:X86 
endif

INSTFILECMD = 	install -D
INSTDIRCMD =	install -d

SRCENVSH =		env.sh

ifeq ($(BUILD),DEBUG)
DEFS +=			-DDEBUG
endif

DEFS +=			-DWIN32 -DWINNT -D_REENTRANT 

INCS += 		-I. -I$(GENDIR)

LIBS += 		gdi32.lib Advapi32.lib User32.lib

## Should this be generic?
# # BLTMODOPTSO =	$(MODSO:%=$(LIBDIR)/%$(SOEXT))

# Local Variables:
# mode: Makefile
# tab-width: 4
# End:
