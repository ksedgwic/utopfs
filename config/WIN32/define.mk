ABSROOT =	$(shell cd $(ROOTDIR) && /usr/bin/pwd)
DIST 	= 	$(ROOTDIR)/../built
ABSDIST = 	$(shell cygpath -a $(DIST))

CURRDIRU =	$(shell pwd)
CURRDIRW =	$(subst \,\\,$(shell cygpath -w $(shell pwd)))
CURRDIRw =	$(wordlist 2, 2, $(subst :, ,$(CURRDIRW)))

OBJEXT =		.obj
SOEXT =			.dll
EXEEXT =		.exe


# It looks ugly enough but that is what MS does in order to get their compiler working in cmd shell. 
# Look at the VSInstallDir\VC\bin\vcvars32.bat.
# 

WINDIRU	= $(shell cygpath -W)
WINDIRW	= $(shell cygpath -Ww)

# Visual Studio 2008 Setup
# TODO: make it more general, support MS C++ Express Setup

VSTUDIO	 			= 	$(subst \Common7\IDE,,$(shell regtool get "\HKLM\Software\Microsoft\VisualStudio\9.0\InstallDir"))

# if not defined look for VS 2005
ifndef VSTUDIO
VSTUDIO	 			= 	$(subst \Common7\IDE,,$(shell regtool get "\HKLM\Software\Microsoft\VisualStudio\8.0\InstallDir"))
endif

# if not defined look for VS .NET 2003
ifndef VSTUDIO
VSTUDIO	 			= 	$(subst \Common7\IDE,,$(shell regtool get "\HKLM\Software\Microsoft\VisualStudio\7.1\InstallDir"))
endif

ifdef VSTUDIO
VSINSTALLDIR		:=	$(VSTUDIO)
VCINSTALLDIR		:=	$(VSTUDIO)\VC
FrameworkDir		:=	$(WINDIRW)\Microsoft.NET\Framework

# framework values are just taken from vs 2009, need to do more work to support earlier versions
FrameworkVersion	:=	v2.0.50727
Framework35Version	:=	v3.5

DevEnvDir			:=	$(VSTUDIO)Common7\IDE

CPPCMDPATH 			= 	$(shell cygpath $(subst \,\\,$(VSTUDIO)))
VSTUDIOU 			= 	$(shell cygpath -m $(subst \,\\,$(VSTUDIO)))

PLATFORM_SDKW	 	= 	$(shell regtool get "\HKCU\Software\Microsoft\Microsoft SDKs\Windows\CurrentInstallFolder")
ifndef PLATFORM_SDKW
PLATFORM_SDKW		= 	$(VSTUDIO)\PlatformSDK
endif
PLATFORM_SDKU	 	= 	$(shell cygpath $(subst \,\\,$(PLATFORM_SDKW)))
PLATFORM_SDKUW	 	= 	$(shell cygpath -m $(subst \,\\,$(PLATFORM_SDKW)))

PATH				:=	$(CPPCMDPATH)Common7/IDE:$(CPPCMDPATH)VC/bin:$(CPPCMDPATH)Common7/Tools:$(WINDIRU)/Microsoft.NET/Framework/v3.5:$(WINDIRU)/Microsoft.NET/Framework/v2.0.50727:$(CPPCMDPATH)VC/VCPackages:$(PLATFORM_SDKU)bin:${PATH}

BASEINCLUDE			=	-I"$(VSTUDIO)VC\atlmfc\include" -I"$(VSTUDIO)VC/include" -I"$(PLATFORM_SDKW)include"

BASELIB				:=	-L $(VSTUDIO)VC\ATLMFC\LIB -L$(VSTUDIO)VC\LIB -L$(PLATFORM_SDKW)LIB

LIBPATH				:=	-LIBPATH:"$(WINDIRW)\Microsoft.NET\Framework\v3.5" \
						-LIBPATH:"$(WINDIRW)\Microsoft.NET\Framework\v2.0.50727" \
						-LIBPATH:"$(VSTUDIO)VC\ATLMFC\LIB" \
						-LIBPATH:"$(VSTUDIO)VC\LIB" \
						-LIBPATH:"$(PLATFORM_SDKW)LIB"

#MANIFEST_EMBED			= 1
SOCMT					= mt
SOCMTFLAGS				= -manifest
SOCMTOUTPUTRESOURCE		= -outputresource
SOC_MANIFEST_BASENAME   = __VC.Debug

endif

CPPCMD 				=  	cl

LIBCMD 				=   lib -OUT:$@

LINK  				=   link
SOEXT 				=	.dll
SOCMD 				=	$(LINK)

LDCMD 				= 	link

ifeq ($(BUILD),DEBUG)
CPPFLAGS =		-Od -Z7 -MDd -W3 -nologo -GF -GR -GT -EHsc
else
CPPFLAGS =		-O2 -Ot -Z7 -MD -W3 -nologo -GF -GR -GT -G7 -EHsc -arch:SSE2 
endif

#RMFILESCMD = del /Q
RMDIRSCMD   = 	rmdir
RMDIRSFLAGS = 	/S /Q


ifeq ($(BUILD),DEBUG)
LDFLAGS =		$(LIBPATH) -nologo -DEBUG -DEBUGTYPE:CV -map
else
LDFLAGS =		$(LIBPATH) -nologo -map
endif

ifeq ($(BUILD),DEBUG)
SOFLAGS =		$(LIBPATH) -nologo -DLL -SUBSYSTEM:WINDOWS -DEBUG \
				-DEBUGTYPE:CV -MAP
else
SOFLAGS =		$(LIBPATH) -nologo -DLL -SUBSYSTEM:WINDOWS -INCREMENTAL:NO -LARGEADDRESSAWARE -TSAWARE:NO -OPT:REF -OPT:ICF /OPT:NOWIN98 -MACHINE:X86 
endif

INSTFILECMD = 	install -D
INSTDIRCMD =	install -d

SRCENVSH =		env.sh

ifeq ($(BUILD),DEBUG)
DEFS +=			-DDEBUG
endif

DEFS +=			-DWIN32 -DWINNT -D_REENTRANT 

INCS += 		$(BASEINCLUDE) -I. -I$(GENDIR)

LIBS += 		gdi32.lib Advapi32.lib User32.lib

## Should this be generic?
# # BLTMODOPTSO =	$(MODSO:%=$(LIBDIR)/%$(SOEXT))

# Local Variables:
# mode: Makefile
# tab-width: 4
# End:
