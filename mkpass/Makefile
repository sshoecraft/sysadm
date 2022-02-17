
#DEBUG=yes
UTIL=yes

include ../Makefile.inc

all: mkpass$(EXE) encfile$(EXE) decfile$(EXE)

mkpass$(EXE): mkpass.c $(DEPS) Makefile 
	$(CC) $(CFLAGS) -o $@ $< $(LIBS) 

encfile$(EXE): encfile.c $(DEPS) Makefile
	$(CC) $(CFLAGS) -o $@ $< $(LIBS) 

decfile$(EXE): decfile.c $(DEPS) Makefile
	$(CC) $(CFLAGS) -o $@ $< $(LIBS) 

install: mkpass$(EXE) encfile$(EXE) decfile$(EXE)
	install -s $? ~/bin

clean:
	rm -f mkpass$(EXE) encfile$(EXE) decfile$(EXE)
