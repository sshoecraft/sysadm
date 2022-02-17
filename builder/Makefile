
BINDIR=$(BUILDROOT)/usr/local/bin
TOOLDIR=$(BUILDROOT)/usr/local/lib/tools
LIBDIR=$(BUILDROOT)/usr/local/lib/builder

SCRIPTS=builder ilocmd
TOOLS=detect_ilo getiso replay.pl locfg.pl create_vms.pl check_ilo detect_drac destroy_vm
FILES=builder_os.dat *.xml *.bin 

all clean:

ifeq ($(strip $(BUILDROOT)),)
SUDO=sudo
PERMS=-o root -g root
endif

install:
	$(SUDO) mkdir -p $(BINDIR) $(LIBDIR) $(TOOLDIR)
	$(SUDO) install -m 755 $(PERMS) $(SCRIPTS) $(BINDIR)
	$(SUDO) install -m 755 $(PERMS) $(TOOLS) $(TOOLDIR)
	$(SUDO) rm -f $(LIBDIR)/ilo*.bin
	$(SUDO) install -m 644 $(PERMS) $(FILES) $(LIBDIR)

deploy:
	rm -rf buildroot
	mkdir -p buildroot
	make install BUILDROOT=./buildroot
	sh mkspec
	rpmbuild --buildroot=`pwd`/buildroot -bb builder.spec
	scp /home/sysadm/rpmbuild/RPMS/noarch/builder-1-0.noarch.rpm ddc-coe:/tmp
	ssh -t ddc-coe "sudo bash -c \"mv /tmp/builder-1-0.noarch.rpm /coe/misc; /usr/local/bin/sync_coe\""
