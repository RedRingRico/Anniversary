## Anniversary Makefile
# Compiler: Hitachi SH Cross Compiler
# Assembler: Hitachi SH Cross Assembler
# Linker: Hitachi SH Cross Linker
# Graphics library: Kamui 2
# System library: Shinobi

# N.B.
# Release is borked, do not try to build

## Ensure the Katana SDK is set
ifeq ($(strip $(KATANA_ROOT)),)
$(error "Please set the KATANA_ROOT path")
endif

ifndef TOPDIR
export TOPDIR		:= $(shell cd .. && cd)
export TOPSRC		= $(TOPDIR)\Source
export TARGETDIR	= $(TOPDIR)\Bin
export OBJSDIR		= $(TOPDIR)\Obj\$(BUILD)
export OBJSABSOLUTE	= $(subst \,/,$(OBJSDIR))
endif

.SILENT :
.SUFFIXES : .c .src .bin .lib .elf

PROJECT := Anniversary
TARGET := $(PROJECT)
OUTFILE = $(TARGETDIR)\$(TARGET).$(EXTENSION)
EXTENSION := elf

LINK_FILES = $(addprefix xlnk.,$(basename $(DEFAULT_OBJS))) $(addprefix lnk.,$(OBJS))

BUILD = Debug

SOURCEDIR = Source

MAKE	:= gmake.exe
ELFCONV	:= elfcnv.exe
ELF2BIN	:= elf2bin.exe
DWFCONV	:= dwfcnv.exe
CC		= shc.exe
ASM		= asmsh.exe
LIBR	= lbr.exe
LINK	= lnk.exe

OPTIMISE_TYPE = size

LIBDIR	= $(KATANA_ROOT)\lib
DEFAULT_INCLUDE	= $(KATANA_ROOT)\include

DEFAULT_LIBS	= kamui2 shinobi sh4nlfzn


DEFAULT_OBJS = strt1.obj strt2.obj systemid.obj toc.obj sg_sec.obj \
	sg_arejp.obj sg_areus.obj sg_areec.obj sg_are00.obj sg_are01.obj \
	sg_are02.obj sg_are03.obj sg_are04.obj sg_ini.obj aip.obj zero.obj

OPTIMISE = 0
#LISTING
#LISTING = -list="$(OBJSDIR)\$*.lst"

COPT_LINK = -g

CDEFS = _KAMUI2_ __SET4__ __SET5__ __KATANA__ _CLX_ _CLX2_ _STRICT_UNION_ _SH4

COPT = -cpu=sh4 -division=cpu -endian=little -fpu=single -round=nearest \
	-pic=0 -macsave=0 -string=const -comment=nonest -sjis \
	-section=p=P,c=C,d=D,b=B -show=obj,source,expansion,w=80,l=0

COPTIMISE = -optimize=$(OPTIMISE)

CFLAGS = $(COPT) $(CDEBUG) $(DEBUGCFLAGS) $(COPT_LINK) $(C_LIST) $(COPTIMISE)

## Assembly flags
ASM_DEBUG = -debug=d

debug: ASM_LIST = -list="$(OBJSDIR)\$*.lst"
relase: ASM_LIST = 
ASM_OPTIONS = -cpu=sh4 -endian=little -sjis -private

ASM_FLAGS = $(ASM_OPTIONS) $(ASM_DEBUG) $(ASM_LIST)

## Linker
LINK_DEBUG = debug
LINK_LIST = 
#print $*.map

L_OPT_LINK = op speed

#if bin
#if opt link
LINK_EXTRA = entry SG_SEC

LINK_START = start IP(8C008000),DSGLH(8C010000)
LINK_FORM = form a
LINK_OPTS = align_section udfcheck elf

LINK_FLAGS = $(LINK_OPTS) $(LINK_DEBUG)

debug: BUILD = Debug
debug: BUILD_TYPE = debug
debug: BUILD_DEF = DEBUG
debug: COPT_LINK =
debug: LINK = lnk.exe
debug: CDEBUG = -debug
debug: TARGET := $(TARGET)D
debug: DEFAULT_LIBS += libcross sh4nlfzn
debug: OPTIMISE = 0
debug: CDEFS += DEBUG
debug: $(TARGET)

release: BUILD = Release
release: BUILD_TYPE = release
release: BUILD_DEF = RELEASE
release: LINK = lbr.exe
release: COPT_LINK = -g
release: CDEBUG = -nodebug
release: EXTENSION = BIN
release: TARGET := 1ST_READ
release: OPTIMISE = 1
release: $(TARGET)

LINK_SCRIPT = "$(OBJSDIR)\lnk.sub"
TRIM_QUOTES = ~

ifneq ($(OBJSABSOLUTE), $(CURDIR))

TARGETDIR:
	-mkdir $(TARGETDIR)

OBJSDIR:
	-mkdir $(OBJSDIR)

CFILES	:= $(foreach dir,$(SOURCEDIR),$(notdir $(wildcard $(dir)/*.c)))
ASMFILES := $(foreach dir,$(SOURCEDIR),$(notdir $(wildcard $(dir)/*.src)))
ICFILES := $(foreach dir,$(SOURCEDIR),$(notdir $(wildcard $(dir)/*.ic)))
export VPATH := $(foreach dir,$(SOURCEDIR),$(CURDIR)/$(dir))
export OBJS := $(CFILES:.c=.obj) $(ASMFILES:.src=.obj) $(ICFILES:.ic=.obj)

# Target re-direction to make in the OBJSDIR directory
$(TARGET): OBJSDIR TARGETDIR
	$(MAKE) -C $(OBJSDIR) -f "$(TOPSRC)\Makefile" $(BUILD_TYPE)
else

# Clear the linker script
LINK_SCRIPT:
	echo.> "$(OBJSDIR)\lnk.sub"

# Target linking
$(TARGET): $(OBJS) LINK_SCRIPT $(LINK_FILES)
	echo $(LINK_START) >> "$(OBJSDIR)\lnk.sub"
	echo $(LINK_FORM) >> "$(OBJSDIR)\lnk.sub"
ifdef LINK_EXTRA
	echo $(LINK_EXTRA) >> "$(OBJSDIR)\lnk.sub"
endif
ifdef LINK_LIST
	echo $(LINK_LIST) >> "$(OBJSDIR)\lnk.sub"
endif
	for %%x in ($(LINK_FLAGS)) do echo %%x >> "$(OBJSDIR)\lnk.sub"
	for %%x in ($(SYSTEM_LIBS) $(DEFAULT_LIBS)) do echo library $(LIBDIR)\%%x.lib >> "$(OBJSDIR)\lnk.sub"
	echo output $(TARGETDIR)\$(TARGET).elf >> "$(OBJSDIR)\lnk.sub"
	echo exit >> "$(OBJSDIR)\lnk.sub"
ifeq ($(OPTIMISE_LINK),1)
	$(LINK) -sub="$(OBJSDIR)\lnk.sub"
else
	$(LINK) -noecho -sub="$(OBJSDIR)\lnk.sub"
endif
ifeq ($(EXTENSION),BIN)
	$(ELF2BIN) -s 8C010000 "$(TARGETDIR)\$(TARGET).elf"
endif
	echo Created $(OUTFILE)

# C file compilation
%.obj: %.c
	echo Compiling $< ...
	echo $(CFLAGS) > "$(OBJSDIR)\shc.sub"
	for %%x in ($(OPTIMISE_TYPE)) do echo -%%x >> "$(OBJSDIR)\shc.sub"
	echo -define= >> "$(OBJSDIR)\shc.sub"
	for %%x in ($(CDEFS) $(EXTRADEFS)) do echo %%$(TRIM_QUOTES)x, >> "$(OBJSDIR)\shc.sub"
	echo __SHC__ -include= >> "$(OBJSDIR)\shc.sub"
	echo ., >> "$(OBJSDIR)\shc.sub"
	echo $(DEFAULT_INCLUDE) >> "$(OBJSDIR)\shc.sub"
ifeq ($(OPTIMISE),1)
	$(CC) $< -ob="$(OBJSDIR)\$*.o" -sub="$(OBJSDIR)\shc.sub"
else
	$(CC) $< -ob="$(OBJSDIR)\$*.o" -sub="$(OBJSDIR)\shc.sub"
	$(DWFCONV) -sysrof="$(OBJSDIR)\$*.obj" "$(OBJSDIR)\$*.o
endif

# Assembly file compilation
$.obj: $.src
	echo Assembling $<
	$(ASM) $< $(ASM_FLAGS) -O="$@"

lnk.% :
	echo input $* >> "$(OBJSDIR)\lnk.sub"
xlnk.% :
	echo input $(LIBDIR)\$*.obj >> "$(OBJSDIR)\lnk.sub"

endif
