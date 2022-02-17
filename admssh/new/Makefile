
#DEBUG=yes
UTIL=yes

include ../Makefile.inc

NAME=syscmd
PROG=$(NAME)$(EXE)
SRCS=$(NAME).c fdutils.c
OBJS=$(SRCS:.c=.o)
BINDIR=/usr/local/bin

all: $(PROG)
	./admssh ddc-coe id

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $(PROG) $(OBJS) $(LIBS)
	ln -sf $(PROG) admssh
	ln -sf $(PROG) admscp

$(OBJS): key.h Makefile

key.h: mkkey Makefile
	./mkkey sysadm

mkkey: mkkey.c
	cc -o mkkey mkkey.c -L/usr/local/lib -lutil

install: $(PROG)
	sudo mkdir -p $(BINDIR)
	sudo install -o root -g root -m 755 $(PROG) admscp $(BINDIR)

clean:
	rm -f $(PROG) $(OBJS)
