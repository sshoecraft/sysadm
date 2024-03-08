
DEBUG=yes

include ../Makefile.inc

NAME=db
LIB=lib$(NAME)$(SUFFIX).a
SRCS=err.c connect.c exec.c fetch.c data.c
OBJS=$(SRCS:.c=.o)

all: $(LIB)

$(OBJS): Makefile

$(LIB): $(OBJS)
	$(AR) cr $(LIB) $(OBJS)

ifeq ($(BITS),64)
_D=64
endif
install: $(LIB)
	sudo install -o root -g root -m 755 $(LIB) $(LIBDIR)$(_D)
	sudo install -o root -g root -m 644 db.h $(INCDIR)

clean:
	rm -f $(OBJS)
