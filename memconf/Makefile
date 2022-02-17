
PROG=memconf

$(PROG): $(PROG).c
	cc -Wall -o $(PROG) $(PROG).c

install: $(PROG)
	sudo mkdir -p /usr/local/lib/tools
	sudo install -o root -g root -m 755 $(PROG) /usr/local/lib/tools

clean:
	rm -f $(PROG)
