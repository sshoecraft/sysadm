
#DEBUG=yes

include ../Makefile.inc

NAME=bgrun
PROG=$(NAME)$(EXE)
SRCS=$(NAME).c
OBJS=$(SRCS:.c=.o)

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $(PROG) $(OBJS) $(LIBOBJS) $(LIBS) 

$(OBJS): Makefile

install: $(PROG)
	sudo install -m 755 -o root -g root $(PROG) /usr/local/bin

clean:
	rm -f $(PROG) $(OBJS)
