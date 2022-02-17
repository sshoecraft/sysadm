
DEBUG=yes
#UTIL=yes
#STATIC=yes
#OS=win
#OS=solaris
#BITS=32

include ../Makefile.inc

NAME=luninfo
PROG=$(NAME)$(EXE)
SRCS=main.c info.c utils.c scsi.c linux.c esx.c win32.c hpux.c solaris.c vg.c dg.c xp.c vv.c
SRCS+=list.c
OBJS=$(SRCS:.c=.o)

ifeq ($(OS),win)
LIBS+=-lsetupapi -luuid
endif

#DEBUG=-DDEBUG
#LINUX=-O -m32 -Wall -Werror -I.
#CFLAGS=-I. -DVOL $(DEBUG) $(LINUX)
#CFLAGS+=-DSCAN_ALL

all: $(PROG)
	sudo ./luninfo -o -l

$(PROG): $(OBJS) $(DEPS)
	$(CC) $(CFLAGS) -o $(PROG) $(OBJS) $(LIBS)

$(OBJS): Makefile version.h luninfo.h scsi.h

clean:
	rm -f $(PROG) $(OBJS) luninfo.c

ifeq ($(strip $(BUILDROOT)),)
SUDO=sudo
PERMS=-o root -g root
endif
BINDIR=$(BUILDROOT)/usr/local/bin

install: $(PROG)
ifneq ($(OS),win)
	$(SUDO) mkdir -p $(BINDIR)
	$(SUDO) $(INSTALL) -m 755 $(PERMS) $(PROG) getluns $(BINDIR)
endif

rpm:
	@sh mkrpm

stamp:
	echo "* `date +"%a %b %d %Y"` Steve Shoecraft (sshoecraft@earthlink.net)" > /tmp/stamp.tmp || exit 1
	v=`cat version.h | awk '{ print $$3 }'`; eval ver=$$v; rel=`cat release`; \
	mv changelog changelog.prev; \
	cat /tmp/stamp.tmp > changelog; \
	echo "version $$ver, release $$rel" >> changelog; \
	cat changelog.prev >> changelog

sync:
	(cd ..; rsync -av --delete luninfo none:)

.PHONY: luninfo.c
#SINSRCS=list.h list.c luninfo.h utils.c main.c info.c linux.c esx.c hpux.c solaris.c scsi.c xp.c vv.c vg.c dg.c
#	cc -o t -Wall -Werror luninfo.c
SINSRCS=list.h luninfo.h scsi.h esx.h vv.h xp.h $(SRCS)
luninfo.c: $(SRCS)
	rm -f luninfo.c
	echo "#include <stdio.h>" >> luninfo.c
	echo "#include <string.h>" >> luninfo.c
	echo "#include <unistd.h>" >> luninfo.c
	echo "#define dprintf(format, args...) /* noop */" >> luninfo.c
	cat version.h >> luninfo.c
	cat $(SINSRCS) | sed "s:#include \"list.h\"::" | \
		sed "s:#include \"esx.h\"::" | \
		sed "s:#include \"vv.h\"::" | \
		sed "s:#include \"xp.h\"::" | \
		sed "s:#include \"scsi.h\"::" | \
		sed "s:#include \"luninfo.h\"::" | \
		sed "s:#include \"version.h\"::" >> luninfo.c
