
#DEBUG=yes
UTIL=yes

include ../Makefile.inc

NAME=admssh
PROG=$(NAME)$(EXE)
SRCS=$(NAME).c fdutils.c
OBJS=$(SRCS:.c=.o)

CFLAGS+=-DETCDIR=\"$$HOME/etc\"

all: $(PROG) mkkey

$(PROG): $(OBJS) Makefile
	$(CC) $(CFLAGS) -o $(PROG) $(OBJS) $(LIBS)
	ln -sf $(PROG) admscp

$(OBJS): key.h

mkkey: mkkey.c
	cc -o mkkey mkkey.c -L/usr/local/lib -lutil
	./mkkey sysadm testing

install: $(PROG)
	mkdir -p ~/bin
	install -m 755 $(PROG) systest ~/bin
	mkdir -p ~/etc
#	install -m 600 hpsa_p01 hpsa_p02 ~/etc
	install -m 600 sysadm ~/etc

clean:
	rm -f $(PROG) $(OBJS)
