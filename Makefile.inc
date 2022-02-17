
ifeq ($(OS),win32)
  OS=win
  BITS=32
endif
ifeq ($(OS),)
  OS=linux
endif

INCDIR?=/usr/local/include
LIBDIR?=/usr/local/lib

#CFLAGS=-I$(INCDIR)

ifeq ($(OS),win)
  ifeq ($(BITS),64)
    ARCH=win64
    BASE=/usr/local/mingw64
    VERS=x86_64-w64-mingw32
    CC=$(BASE)/bin/x86_64-w64-mingw32-gcc
    AR=$(BASE)/bin/x86_64-w64-mingw32-ar
    STRIP=$(BASE)/bin/x86_64-w64-mingw32-strip
  else
    ARCH=win32
    BITS=32
    BASE=/usr/bin/
    VERS=i386-mingw32
    CC=/usr/bin/i686-w64-mingw32-gcc
    AR=/usr/bin/i686-w64-mingw32-gcc-ar
#    STRIP=$(BASE)/bin/i386-mingw32-strip
    #CFLAGS+=-DWIN32_LEAN_AND_MEAN
  endif
  PREFIX=$(BASE)/$(VERS)
  EXE=.exe
  INSTALL=install
else
  ifeq ($(OS),linux)
	MACH := $(shell uname -m)
	ifeq ($(BITS),)
		ifeq ($(MACH),x86)
			BITS=32
		else
			ifeq ($(MACH),x86_64)
				BITS=64
			else
				ifeq ($(MACH),i686)
					BITS=32
				endif
			endif
		endif
	endif
	try := $(shell echo blah)
	ifeq ($(BITS),32)
	    ARCH=linux32
	    ifeq ($(MACH),x86_64)
		CFLAGS+=-m32
	    endif
	else
	    ARCH=linux64
	endif
	CC=$(shell if test -x /usr/bin/gcc4; then echo gcc4; else echo gcc; fi)
	AR=ar
	STRIP=strip
	INSTALL=install
  else
	ifeq ($(OS),solaris)
	MACH=$(shell uname -p)
	CC=$(shell if test -x /opt/csw/gcc4/bin/gcc; then echo /opt/csw/gcc4/bin/gcc; else echo gcc; fi)
	ARCH=solaris_$(MACH)
	INSTALL=ginstall
  else
	ARCH=unk
	MACH=unk
	OS=unk
	endif
  endif
endif

TOOL_NAME=$(NAME)_$(ARCH)$(DEBUG_SUFFIX)$(EXE)

CFLAGS+=-Wall

SRC_ROOT ?= ..

# Debugging?
ifeq ($(DEBUG),yes)
CFLAGS+=-DDEBUG -g
DEBUG_SUFFIX=_debug
else
CFLAGS+=-O2 -pipe
endif

# Static binary?
ifeq ($(STATIC),yes)
CFLAGS+=-static
endif

# Suffixes
SUFFIX=_$(ARCH)$(DEBUG_SUFFIX)

# Uses LIBOS?
ifeq ($(LIBOS),yes)
CFLAGS+=-I$(SRC_ROOT)/libos
LIBS+=-L$(SRC_ROOT)/libos -los$(SUFFIX)
DEPS+=$(SRC_ROOT)/libos/libos$(SUFFIX).a
endif

# Uses VIM?
ifeq ($(VIM),yes)
UTIL=yes
SOAP=yes
#CFLAGS+=-I$(SRC_ROOT)/vim -I$(SRC_ROOT)/vim/soapfuncs
CFLAGS+=-I$(SRC_ROOT)/vim
LIBS+=-L$(SRC_ROOT)/vim -lvim$(SUFFIX)
DEPS+=$(SRC_ROOT)/vim/libvim$(SUFFIX).a
endif

# Uses VIMX?
ifeq ($(VIMX),yes)
UTIL=yes
SOAP=yes
CFLAGS+=-I$(SRC_ROOT)/vimx
LIBS+=-L$(SRC_ROOT)/vimx -lvimx$(SUFFIX)
DEPS+=$(SRC_ROOT)/vimx/libvimx$(SUFFIX)
endif

# Uses libutil?
ifeq ($(UTIL),yes)
ifneq ($(wildcard $(SRC_ROOT)/util),)
	CFLAGS+=-I$(SRC_ROOT)/util
	LIBS+=-L$(SRC_ROOT)/util -lutil$(SUFFIX)
	DEPS+=$(SRC_ROOT)/util/libutil$(SUFFIX).a
else
	LIBS+=-lutil$(SUFFIX)
	DEPS+=$(LIBDIR)/libutil$(SUFFIX).a
endif
endif

# Uses SOAP?
ifeq ($(SOAP),yes)
SOAP_CFLAGS=-DWITH_OPENSSL -DWITH_DOM -DWITH_COOKIES -DWITH_GZIP
CFLAGS+=-I$(SRC_ROOT)/soap
LIBS+=-L$(SRC_ROOT)/soap -lsoap$(SUFFIX)
DEPS+=$(SRC_ROOT)/soap/libsoap$(SUFFIX).a
#LIBS+=-lgsoapssl$(DEBUG_SUFFIX) -lz -lssl -lcrypto
LIBS+=-lgsoapssl -lz -lssl -lcrypto
#LIBS+=-lz -lssl -lcrypto
#LIBS+=-lz ~/lib/libssl.a ~/lib/libcrypto.a -ldl
endif

# Uses ODBC?
ifeq ($(ODBC),yes)
ifneq ($(wildcard $(SRC_ROOT)/db),)
	CFLAGS+=-I$(SRC_ROOT)/db
	LIBS+=-L$(SRC_ROOT)/db -ldb$(SUFFIX)
	DEPS+=$(SRC_ROOT)/db/libdb$(SUFFIX).a
else
	LIBS+=-ldb$(SUFFIX)
	DEPS+=$(LIBDIR)/libdb$(SUFFIX).a
endif
ifeq ($(OS),win)
LIBS+=-lodbc32
else
LIBS+=-lodbc -ldl
endif
endif

# Uses MySQL?
ifeq ($(MYSQL),yes)
LIBS+=-lmysqlclient
endif

# Uses libxml?
ifeq ($(XML),yes)
CFLAGS+=$(shell xml2-config --cflags)
LIBS+=$(shell xml2-config --libs)
endif

# Uses pthreads?
ifeq ($(PTHREADS),yes)
ifeq ($(OS),win)
LIBS+=-lpthreadGC2
else
LIBS+=-lpthread
endif
endif

# Needed for win
ifeq ($(OS),win)
LIBS+=-lws2_32
ifeq ($(BITS),32)
#LIBS+=-luserenv -lgdi32 -lregex -lws2_32 -lmsvcrt
LIBS+=-luserenv -lgdi32 -lws2_32 -lmsvcrt
endif
endif

#ifeq ($(ADDLIBDIR),win)
#	NEWLIBS=-L$(LIBDIR) $(libs)
#	LIBS=$(NEWLIBS)
#endif

#.SUFFIXES: .c .o$(SUFFIX)
#.c.o$(SUFFIX):
.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAGS) -c $< -o $@
