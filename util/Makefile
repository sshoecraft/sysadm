
DEBUG=no

include ../Makefile.inc

NAME=util
LIB=lib$(NAME)$(SUFFIX).a
#LIB=lib$(NAME).a
SRCS=bindump.c list.c trim.c stredit.c strele.c getpass.c fdutils.c socket.c dns.c encode.c encrypt.c
ifneq ($(OS),win)
SRCS+=getopt.c
endif
OBJS=$(SRCS:.c=.o)

all: $(LIB)

socket.o: socket.h

$(OBJS): util.h Makefile

$(LIB): $(OBJS)
	$(AR) cr $(LIB) $(OBJS)

ifeq ($(BITS),64)
_D=64
endif
install: $(LIB)
#	sudo install -o root -g root -m 755 $(LIB) /usr/local/lib$(_D)
#	sudo install -o root -g root -m 755 *.h /usr/local/include

clean:
	rm -f $(OBJS)
