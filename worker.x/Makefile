
#DEBUG=yes
UTIL=yes
PTHREAD=yes
#BITS=32
#STATIC=yes

include ../Makefile.inc

NAME=worker
PROG=$(NAME)$(EXE)
SRCS=main.c worker.c exec.c
OBJS=$(SRCS:%.c=%.o)
ifneq ($(ARCH),win32)
LIBS+=-lpthread
endif

all: $(PROG)
#	echo "/" | ./worker - 8 /bin/ls -lFA
#	cat input | wc -l; ./worker input 128 /bin/echo | grep ^input | wc -l
#	./worker input 4 ./t

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $(PROG) $(OBJS) $(LIBS)

$(OBJS): worker.h Makefile

clean:
	rm -f $(PROG) $(OBJS)

install: $(PROG)
ifeq ($(OS),linux)
	sudo install -m 755 -o bin -g bin $(NAME) /usr/local/bin
endif
#	sudo install -o root -g root -m 755 $(PROG) /var/www/html/tools/$(NAME)_$(ARCH)$(EXE)

test::
	@sh ./test

rpm: install
	@sh mkrpm

stamp:
	echo "* `date +"%a %b %d %Y"` Steve Shoecraft (stephen.shoecraft@hp.com)" >> changelog
	echo "" >> changelog
