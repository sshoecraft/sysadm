
#DEBUG=yes
#ifneq ($(OS),win)
#XML=yes
#endif

include ../Makefile.inc

NAME=xmlcat
PROG=$(NAME)$(EXE)
SRCS=$(NAME).c
OBJS=$(SRCS:.c=.o)
ifeq ($(OS),linux)
CFLAGS+=-I/usr/include/libxml2
LIBS+=-lxml2
endif

all: $(PROG)
#all:

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $(PROG) $(OBJS) $(LIBOBJS) $(LIBS)

$(OBJS): Makefile

install: $(PROG)
ifeq ($(OS),linux)
	sudo install -m 755 -o root -g root $(PROG) /usr/local/bin
endif
#	sudo install -m 755 -o root -g root $(PROG) /var/www/html/tools/$(NAME)_$(ARCH)$(EXE)

clean:
	rm -f $(PROG) $(OBJS)
