
LIBDIR=/usr/local/lib/tools
FILES=admssh admscp key.gz

all clean:

install:
	sudo mkdir -p $(LIBDIR)
	sudo install -m 755 -o root -g root $(FILES) $(LIBDIR)
	sudo install -m 755 -o root -g root $(FILES) /usr/local/bin
