
NAME=worker
SRCS=main.c worker.c exec.c
LIBS=-lpthread

CFLAGS=-Wall -m32 -I. -I../utils
#CFLAGS+=-DDEBUG
OBJS=$(SRCS:%.c=%.o)

all: $(NAME)
#	./$(NAME) -h
#	echo "/tmp" | ./worker - 1 ls +lFA

$(NAME): $(OBJS) Makefile
	cc -static $(CFLAGS) -o $@ $(OBJS) $(LIBS)

.SUFFIXES: .o .c
.c.o:
	${CC} ${CFLAGS} -o $@ -c $<

$(OBJS): worker.h Makefile

clean:
	rm -f $(NAME) $(OBJS)

install: $(NAME)
ifeq ($(OS),linux)
	sudo install -m 755 -o bin -g bin $(NAME) /usr/local/bin
endif
	sudo install -o root -g root -m 755 $(PROG) /var/www/html/tools/$(NAME)_$(ARCH)$(EXE)

rpm:
	@sh mkrpm

stamp:
	echo "* `date +"%a %b %d %Y"` Steve Shoecraft (stephen.shoecraft@hp.com)" >> worker.spec
	echo "" >> worker.spec
