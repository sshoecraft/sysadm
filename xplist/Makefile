
DEBUG=yes
#UTIL=yes
#STATIC=yes
#OS=win
#BITS=32

include ../Makefile.inc

NAME=xplist
PROG=$(NAME)$(EXE)
SRCS=main.c xp.c esx.c vg.c dg.c utils.c linux.c hpux.c win32.c common.c
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
#	pbrun ksh -c './xplist -d'
#	sudo ./xplist
#	scp xplist $(ESX_SERVER):/tmp
#	time ssh -n -q -o BatchMode=yes $(ESX_SERVER) "sudo /tmp/xplist -n"
#	scp xplist $(VG_SERVER):/tmp
#	time ssh -n -q -o BatchMode=yes $(VG_SERVER) "sudo /tmp/xplist -n"
#	scp xplist $(DG_SERVER):/tmp
#	time ssh -n -q -o BatchMode=yes $(DG_SERVER) "sudo /tmp/xplist -n"
#	scp -i ~/.ssh/stevexp.key $(PROG) Steve@stevexp:/data
#	ssh -i ~/.ssh/stevexp.key -n -q -o BatchMode=yes Steve@stevexp "/data/xplist"

$(PROG): $(OBJS) $(DEPS)
	$(CC) $(CFLAGS) -o $(PROG) $(OBJS) $(LIBS)

$(OBJS): Makefile version.h xplist.h

clean:
	rm -f $(PROG) $(OBJS)

ifeq ($(strip $(BUILDROOT)),)
SUDO=sudo
PERMS=-o root -g root
endif
BINDIR=$(BUILDROOT)/usr/local/bin

install: $(PROG)
ifeq ($(OS),linux)
	$(SUDO) mkdir -p $(BINDIR)
	$(SUDO) install -m 755 $(PERMS) $(PROG) getxp $(BINDIR)
endif
#	sudo install -m 755 -o root -g root $(PROG) /var/www/html/tools/$(NAME)_$(ARCH)$(EXE)


#	worker /home/sysadm/data/lists/allservers 128 ./doinst 2>/dev/null
installall:
	sh ./install.sh

rpm:
	@sh mkrpm

stamp:
	echo "* `date +"%a %b %d %Y"` Steve Shoecraft (stephen.shoecraft@hp.com)" > /tmp/stamp.tmp || exit 1
	v=`cat version.h | awk '{ print $$3 }'`; eval ver=$$v; rel=`cat release`; \
	mv changelog changelog.prev; \
	cat /tmp/stamp.tmp > changelog; \
	echo "version $$ver, release $$rel" >> changelog; \
	cat changelog.prev >> changelog

sync:
	(cd ..; rsync -av --delete xplist d9u0018c:)

.PHONY: xplist.c
XPSRCS=list.h list.c utils.c xplist.h main.c xp.c vg.c dg.c esx.c
xplist.c: $(XPSRCS)
	rm -f xplist.c
	echo "#include <stdio.h>" >> xplist.c
	echo "#include <unistd.h>" >> xplist.c
#	echo "#define dprintf(m) /* noop */" >> xplist.c
	cat version.h >> xplist.c
	cat $(XPSRCS) | sed "s:#include \"list.h\"::" | \
		sed "s:#include \"xplist.h\"::" | \
		sed "s:#include \"version.h\"::" >> xplist.c
	cc -o t -Wall -Werror xplist.c
