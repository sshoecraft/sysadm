
BINDIR=/usr/local/bin
LIBDIR=/usr/local/lib/tools

FILES=temproot
LIB_FILES=dotemproot

all:

install:
	sudo install -m 755 -o root -g root $(FILES) $(BINDIR)
	sudo mkdir -p $(LIBDIR)
	sudo install -m 755 -o root -g root $(LIB_FILES) $(LIBDIR)
