
BINDIR=/usr/local/bin
LIBDIR=/usr/local/lib/tools

FILES=getcons getmc chkluns whichoa mailme htmailme chkcons resetblade doeject listoa oalist oacmd cold addilouser getfq fixalt setfarm powerop vcip getwwns
LIB_FILES=oalist.exp oareset.exp sexec oacmd.exp vccmd.exp oanames.exp listoa.exp *.xml acmd ascp wwninfo holdpwrbtn.xml

all:

#	for f in $(FILES);  do echo $$f; done
install:
	for f in $(FILES) $(LIB_FILES); do sed -i -e 's:testing=1:testing=0:' $$f; done
#	for f in $(FILES) $(LIB_FILES); do sed -i -e 's:verbose=1:verbose=0:' $$f; done
	sudo install -m 755 -o root -g root $(FILES) $(BINDIR)
	sudo mkdir -p $(LIBDIR)
	sudo install -m 755 -o root -g root $(LIB_FILES) $(LIBDIR)
