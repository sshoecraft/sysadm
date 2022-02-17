
#DEBUG=yes

include ../../Makefile.inc

NAME=worker
PROG=$(NAME)$(EXE)
SRCS=main.c worker.c
OBJS=$(SRCS:%.c=%.o)
LIBS=-lpthread

all: $(NAME)
#	echo "/" | ./worker - 8 /bin/ls -lFA
#	cat input | wc -l; ./worker input 128 /bin/echo | grep ^input | wc -l
#	./worker input 2 ./t

$(PROG): $(OBJS) Makefile
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

$(OBJS): worker.h Makefile

clean:
	rm -f $(NAME) $(OBJS)

install: $(NAME)
ifeq ($(OS),linux)
	sudo install -m 755 -o bin -g bin $(NAME) /usr/local/bin
endif
	sudo install -o root -g root -m 755 $(PROG) /var/www/html/tools/$(NAME)_$(ARCH)$(EXE)

rpm: install
	@sh mkrpm

stamp:
	echo "* `date +"%a %b %d %Y"` Steve Shoecraft (stephen.shoecraft@hp.com)" >> changelog
	echo "" >> changelog
