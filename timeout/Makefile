
#DEBUG=yes

include ../Makefile.inc

NAME=timeout
PROG=$(NAME)$(EXE)
SRCS=$(NAME).c
OBJS=$(SRCS:.c=.o)

all: $(PROG)

$(OBJS): Makefile

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $(PROG) $(OBJS) $(LIB)

install: $(PROG)
ifeq ($(OS),linux)
	sudo install -m 755 -o bin -g bin $(NAME) /usr/local/bin
endif
	sudo install -o root -g root -m 755 $(PROG) /var/www/html/tools/$(NAME)_$(ARCH)$(EXE)


test:
	./$(PROG) 10 ssh g3t0015 "sleep 60"

clean:
	rm -f $(PROG) $(OBJS)
