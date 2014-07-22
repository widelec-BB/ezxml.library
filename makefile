
SHELL	=	/bin/sh

##############################################################################

CODETYPE	=	PPC
VERSION		=	0

OUT = ezxml.library

##############################################################################

G_IPATH    = -I./os-include
G_DEFINES  = -noixemul -DUSE_INLINE_STDARG -D__AMIGADATE__=\"\ \($(shell date "+%d.%m.%y")\)\ \"
G_OPTFLAGS = -O2 \
	-w \
	-fomit-frame-pointer \
	-fverbose-asm \
	-mno-prototype \
	-mcpu=604e \
	-mregnames \
	-Wformat \
	-Wunused \
	-Wuninitialized	\
	-Wconversion \
	-Wstrict-prototypes	\
#  -D__DEBUG__ \
	-Werror-implicit-function-declaration

all: os-include/ppcinline/ezxml.h \
     os-include/proto/ezxml.h \
     $(OUT) \
     libezxml_shared.a \
     test

clean:
	rm -f $(OUT).elf $(OUT).db $(OUT).dump test	*.o *.a os-include/ppcinline/ezxml.h os-include/proto/ezxml.h
	rm -rf doc/*

.c.o:
	ppc-morphos-gcc $(G_CFLAGS) $(G_OPTFLAGS) $(G_DEBUG) $(G_DEFINES) $(G_IPATH) -o $*.o -c $*.c

GLOBAL = debug.h libdata.h os-include/libraries/ezxml.h

lib.o:		   $(SRC)lib.c		   $(GLOBAL) ezxml.library.h
libfunctions.o:	   $(SRC)libfunctions.c		   $(GLOBAL)
libfunctable.o:	   $(SRC)libfunctable.c		   $(GLOBAL)

OBJS = lib.o \
	libfunctions.o \
	libfunctable.o

os-include/ppcinline/ezxml.h: os-include/fd/ezxml_lib.fd os-include/clib/ezxml_protos.h
	@mkdir -p os-include/ppcinline
	cvinclude.pl --fd os-include/fd/ezxml_lib.fd --clib os-include/clib/ezxml_protos.h --inline $@

os-include/proto/ezxml.h: os-include/fd/ezxml_lib.fd
	@mkdir -p os-include/proto
	cvinclude.pl --fd os-include/fd/ezxml_lib.fd --proto $@

lib_glue.a: os-include/clib/ezxml_protos.h os-include/fd/ezxml_lib.fd os-include/ppcinline/ezxml.h os-include/proto/ezxml.h
	cvinclude.pl --fd os-include/fd/ezxml_lib.fd --clib os-include/clib/ezxml_protos.h --gluelib lib_glue.a

libezxml_shared.a: lib_shared.o lib_glue.a
	@-rm -f libezxml_shared.a
	cp lib_glue.a libezxml_shared.a
	$(AR) cru libezxml_shared.a lib_shared.o
	ppc-morphos-ranlib libezxml_shared.a

test.o: test.c os-include/ppcinline/ezxml.h os-include/proto/ezxml.h

$(OUT): $(OBJS)
	ppc-morphos-ld -fl libnix $(OBJS) -o $(OUT).db -lc
# -ldebug
	ppc-morphos-strip -o $(OUT).elf --remove-section=.comment $(OUT).db


bump:
	bumprev2 VERSION $(VERSION) FILE $(OUT) TAG $(OUT) ADD "© 2011 by Filip Maryjañski, written by Aaron Voisine"


dump:
	ppc-morphos-objdump --section-headers --all-headers --reloc --disassemble-all $(OUT).db >$(OUT).dump


install: all
	@mkdir -p /sys/libs/
	cp $(OUT).elf /sys/libs/$(OUT)
	cp libezxml_shared.a /gg/ppc-morphos/lib/libezxml.a
	@-flushlib $(OUT)

test: test.o
	ppc-morphos-gcc test.o -o test -noixemul -lc -lm

doc/ezxml.doc: libfunctions.c
	@robodoc >NIL: libfunctions.c doc/ezxml.doc ASCII SORT TOC TABSIZE 2

doc/ezxml.guide: libfunctions.c
	@robodoc >NIL: libfunctions.c doc/ezxml.guide GUIDE SORT TABSIZE 2

doc/ezxml.html: libfunctions.c
	@robodoc >NIL: libfunctions.c doc/ezxml.html HTML SORT TABSIZE 2

doc: doc/ezxml.doc doc/ezxml.guide doc/ezxml.html
	@echo "Documentation generated"

