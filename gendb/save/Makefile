
#DEBUG=yes
ODBC=yes
UTIL=yes

include ../Makefile.inc

NAME=gendb
PROG=$(NAME)$(EXE)
SRCS=main.c info.c c_inc.c c_src.c php_src.c funcs.c util.c
OBJS=$(SRCS:.c=.o)

all: $(PROG)
#	./$(PROG) -s cio -u dcc_ro -p dcc masterlinkage
#	./gendb -s mbs -u mbs_ro -p "mbs2011readonly**" dbo.MBS && cc -c gendb_mbs.c && rm -f gendb_mbs.*
#	./gendb -s dashboard -u wsm_rw -p wsm vchostutil
#	./gendb -n id,last_seen farms server+name
#	./gendb -s rmdb -u smdb_ro -p smdb

$(OBJS): Makefile

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $(PROG) $(OBJS) $(LIBS) 

install: $(PROG)
ifeq ($(OS),linux)
	sudo install -m 755 -o root -g root $(PROG) /usr/local/bin
endif
	sudo install -m 755 -o root -g root $(PROG) /var/www/html/tools/$(NAME)_$(ARCH)$(EXE)

clean:
	rm -f $(PROG) $(OBJS)
