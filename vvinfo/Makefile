
#OS=win
#BITS=32
#DEBUG=yes

include ../Makefile.inc

NAME=vvinfo
PROG=$(NAME)$(EXE)
SRCS=vvinfo.c
OBJS=$(SRCS:.c=.o)

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $(PROG) $(OBJS) $(LIBS)

$(OBJS): Makefile 

install: $(PROG)
ifeq ($(OS),linux)
	sudo install -m 755 -o bin -g bin $(NAME) getvv /usr/local/bin
endif
	sudo install -o root -g root -m 755 $(PROG) /var/www/html/tools/$(NAME)_$(ARCH)$(EXE)

clean:
	rm -f $(PROG) $(OBJS)

rpm:
	@sh mkrpm

stamp:
	echo "* `date +"%a %b %d %Y"` Steve Shoecraft (stephen.shoecraft@hp.com)" > /tmp/stamp.tmp || exit 1
	v=`cat version.h | awk '{ print $$3 }'`; eval ver=$$v; rel=`cat release`; \
	mv changelog changelog.prev; \
	cat /tmp/stamp.tmp > changelog; \
	echo "version $$ver, release $$rel" >> changelog; \
	cat changelog.prev >> changelog

