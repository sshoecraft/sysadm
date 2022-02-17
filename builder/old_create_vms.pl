#!/usr/bin/perl
use strict;
use warnings;
use Data::Dumper;
use XML::Simple;
use VMware::VIRuntime;
use feature "switch";

my $debug = 3;
sub dprintf { my $level = shift; if ($debug >= $level) { print "#"; printf(@_); } }

$Util::script_version = "1.0";

my %opts = (
	file => {
		type => "=s",
		help => "config file path",
		required => 0,
	},
);

# read/validate options and connect to the server
Opts::add_options(%opts);
Opts::parse();
Opts::validate();

sub load_config {
	my ($fn) = @_;
	if ($fn) {
		return XMLin($fn, KeyAttr => [ ], forcearray => [ 'disk','nic' ], GroupTags => { disks => 'disk', nics => 'nic' });
	}
	else {
		warn "Reading VM configs from standard input\n";
		my (@data) = Load(join "", <STDIN>);
		close STDIN;
		return @data;
	}
}

# Calculate the total disk size required, assuming thick disks, and including
# swap file
sub total_vm_size {
	my ($vm) = @_;
	my $size;
	for my $disk (@{$vm->{disks}}) {
		$size += $disk->{size};
	}
	$size += $vm->{memorySizeMB};
}

# Get the next available unit number... Hmm. Should be changed to object so each VM can have a new one.
{
	my $unit = 0;
	sub unit_number { $unit++ }
	sub reset_unit_number { $unit = 0 };
}

# Compares two arrays, $src and $tar.
# If $src has more elements than $tar, return -1
# If there is a difference in array elements, returns 0
# If $tar is bigger than $src, but all of $src elements match $tar, return 1
# return 2 if they are the same
sub match_array {
	my ($src, $tar) = @_;
	if (@{$src} > @{$tar}) {
		return -1;
	}
	for (my $i = 0; $i < @{$tar}; $i++) {
		# $tar is bigger than $src, can't match - but  might in the future
		if ($i >= @{$src}) {
			return 1;
		}

		# Element of $a is not equal to $b
		if ($src->[$i] ne $tar->[$i]) {
			return 0;
		}
	}
	return 2;
}

my $doexit = 0;
# Find a folder (stored as an array ref of the path components) from a root
# $folder. The current path is the array ref $current.
sub find_folder {
	my ($folder, $current, $target) = @_;

	dprintf(3,"folder: %s\n", $folder->name);
	for my $c (@$current) { dprintf(3,"c: %s\n", $c); }

	my @current = (@{$current}, $folder->name);
	my $match = match_array(\@current, $target);

	dprintf(3,"match: %s\n", $match);
	if ($match <= 0) {
		return;
	}
	# On the right track...
	elsif ($match == 1) {
		$folder->can("childEntity") or return;
		my $children = $folder->childEntity || return;
		for my $child (@{$children}) {
			print Dumper($child);
			next if $child->type eq 'VirtualMachine';
			my $res = find_folder( Vim::get_view(mo_ref => $child), \@current, $target );
			return $res if $res;
		}
	}      
	# We have a winner!
	else {
		return $folder;
	}
}

# Find a compute resource at $path
sub get_host {
	my ($dc, $path) = @_;

#	printf("ref: %s\n", ref $cluster);
#	dprintf(1,"get_host: cluster: %s\n", $cluster->name);

	if ($path) {
		return find_folder(
			Vim::get_view(mo_ref => $dc->hostFolder),
			[], $path
		);
	}
	# Pick one at random
	else {
		die "TODO: Not yet implemented!\n";
	}
}

sub get_vmfolder {
	my ($dc, $path) = @_;

#	dprintf(1,"get_vmfolder: dc: %s, path: %s\n", $dc, $path);
	$path //= [ "vm" ];
	$doexit = 1;
	return find_folder(
		Vim::get_view(mo_ref => $dc->vmFolder), [], $path
	);
}

sub get_coe_install {
	my ($host) = @_;

	my @datastores = @{ Vim::get_views(mo_ref_array => $host->datastore) };
	my ($target) = grep { $_->summary->name =~ /coe-images/} @datastores;
	printf("type: %s\n", ref($target));
	if ($target) {
		printf("name: %s\n", $target->summary->name);
		return $target->summary->name;
	}
#	foreach my $ds (@datastores) { printf("%s\n", $ds->name); }
}

# Check the datastore exists on the host, if no datastore is given, return the
# store with the greatest free space
sub get_ds_name {
	my ($host, $store) = @_;

	my @datastores = @{ Vim::get_views(mo_ref_array => $host->datastore) };
	if ($store) {
		foreach my $ds (@datastores) { printf("%s\n", $ds->name); }
		my ($target) = grep { $_->summary->name eq $store } @datastores;
		if ($target) {
			printf("name: %s\n", $target->summary->name);
			return $target->summary->name;
		}
	}
	else {
		my ($target) = sort { $b->summary->freeSpace <=> $a->summary->freeSpace } @datastores;
		return $target->summary->name;
	}
}

# Return the MO_ref for a datastore $name on $host
sub get_ds_mo {
	my ($host, $path) = @_;
	my ($ds) = ($path =~ /^\[(.*?)\]/);
	$ds || die "No datastore found in path '$path'\n";

	my @datastores = @{ Vim::get_views(mo_ref_array => $host->datastore) };

	my ($target) = grep { $_->summary->name eq $ds } @datastores;
	if ($target) {
		return $target->{mo_ref}
	}
	else {
		die "Couldn't find datastore $ds\n";
	}
}

sub create_ide_ctl {
	my ($vm) = @_;
	my $ctl = VirtualIDEController->new(
		key => int(200), # Magic value...
		device => [3000],
		busNumber => 0,
	);

	return VirtualDeviceConfigSpec->new(
		device => $ctl,
		operation => VirtualDeviceConfigSpecOperation->new('add')
	);
}

# Create the controller configuraton
# TODO: Change controller type depending on cfg
sub create_ctl {
	my ($vm) = @_;

	my $ctl;
	if ($vm->{guestId} =~ m/rhel6/) {
		$ctl = ParaVirtualSCSIController->new(
			key => 0,
			device => [0],
			busNumber => 0,
			sharedBus => VirtualSCSISharing->new('noSharing')
		);
	} else {
		$ctl = VirtualLsiLogicController->new(
			key => 0,
			device => [0],
			busNumber => 0,
			sharedBus => VirtualSCSISharing->new('noSharing')
		);
	}

	return VirtualDeviceConfigSpec->new(
		device => $ctl,
		operation => VirtualDeviceConfigSpecOperation->new('add')
	);
}

# Create the disk configuration
sub create_disks {
	my ($vm, $ds) = @_;

	my @disks;
	for my $disk (@{$vm->{disks}}) {
		my $backing = VirtualDiskFlatVer2BackingInfo->new(
			thinProvisioned => $disk->{thin},
			diskMode => 'persistent',
			fileName => $ds
		);

		my $disk = VirtualDisk->new(
			backing => $backing,
			controllerKey => 0,
			key => 0,
			unitNumber => unit_number(),
			capacityInKB => $disk->{size}*1024,
		);

		push @disks, VirtualDeviceConfigSpec->new(
			device => $disk,
			fileOperation => VirtualDeviceConfigSpecFileOperation->new('create'),
			operation => VirtualDeviceConfigSpecOperation->new('add')
		);
	}
	return @disks;
}

sub create_cdrom {
	my ($vm, $ctl, $host) = @_;
	my @devices;
	my $backing;
	if ($vm->{cdrom}{path}) {
		my $path = $vm->{cdrom}->{path};
		$backing = VirtualCdromIsoBackingInfo->new(
			datastore => get_ds_mo( $host, $path ),
			fileName => $path,
		);
	}
	else {
		$backing = VirtualCdromRemotePassthroughBackingInfo->new(
			deviceName => "",
			exclusive => 0,
		);
	}

	my $cdrom = VirtualCdrom->new(
		backing => $backing,
		connectable => VirtualDeviceConnectInfo->new(
			allowGuestControl => 1,
			connected => 0,
			startConnected => $vm->{cdrom}->{connected},
		),
		controllerKey => int( $ctl->device->key ),
		unitNumber => 0,
		key => int(3000), # Magic value...
	);
		
	push @devices, VirtualDeviceConfigSpec->new(
		device => $cdrom,
		operation => VirtualDeviceConfigSpecOperation->new('add')
	);
	return @devices;
}

# Create the NIC configuration
sub create_nics {
	my ($vm, $host) = @_;
#	my %networks = map { $_->name => $_ } @{ Vim::get_views(mo_ref_array => $host->network) };
	my @host_networks = @{ Vim::get_views(mo_ref_array => $host->network) };
	my %networks = map { $_->name => $_ } @host_networks;

#	my $net_view = Vim::get_views(mo_ref_array => $host->network);

	my @nics;
	for my $nic (@{$vm->{nics}}) {
		unless($networks{ $nic->{network} }) {
			for my $net (@host_networks) {
				if ($net->{summary}->{name} =~ m/$nic->{network}/) {
					$nic->{network} = $net->{summary}->{name};
					last;
				}
			}
		}

		# TODO: Exit more cleanly
		exists $networks{ $nic->{network} } || die "No such network: ".$nic->{network}."\n";

		my $network = $networks{ $nic->{network} };

		my $backing;
		if ($network->{summary}->{network}->{type} eq "DistributedVirtualPortgroup") {
			my $dvs = Vim::get_view(mo_ref => $network->{config}->{distributedVirtualSwitch});
			$backing = VirtualEthernetCardDistributedVirtualPortBackingInfo->new(
				port => DistributedVirtualSwitchPortConnection->new(
					portgroupKey => $network->{key},
					switchUuid => $dvs->{uuid}
				)
			);
		} else {
			$backing = VirtualEthernetCardNetworkBackingInfo->new(
				deviceName => $nic->{network},
				network => $networks{ $nic->{network} },
			);
		}

		my $connection = VirtualDeviceConnectInfo->new(
			allowGuestControl => 1,
			connected => 0,
			startConnected => $nic->{connected},
		);

		my @args = (
			backing => $backing,
			key => 0,
			unitNumber => unit_number(),
			addressType => 'generated',
			connectable => $connection,
		);

		my $device;
		given ($nic->{type}) {
			when (undef) { $device = VirtualE1000->new(@args) }
			when ("e1000") { $device = VirtualE1000->new(@args) }
			when ("pcnet32") { $device = VirtualPCNet32->new(@args) }
			when ("vmxnet") { $device = VirtualVmxnet->new(@args) }
			when ("vmxnet2") { $device = VirtualVmxnet2->new(@args) }
			when ("vmxnet3") { $device = VirtualVmxnet3->new(@args) }
		}

		push @nics, VirtualDeviceConfigSpec->new(
			device => $device,
			operation => VirtualDeviceConfigSpecOperation->new('add')
		);
	}

	return @nics;
}

# Get the datacenter
sub get_dc {
	my ($datacenter) = @_;

	my @dc_view = @{ Vim::find_entity_views(
		view_type => 'Datacenter',
		defined $datacenter ? (filter => { name => $datacenter }) : (),
	) };

	return shift @dc_view;
}

# Get the folder to put the VM in
sub get_folder {
	my ($dc, $vm) = @_;

	if ($vm) {
	}
	else {
		return Vim::get_view(mo_ref => $dc->vmFolder);
	}
}

sub do_defaults {
	my ($vm) = @_;
	my %defaults = (
		memorySizeMB => 2048,
		version => "vmx-08",
		numCpu => 2,
		guestId => "otherGuest64",
		cdrom => {
			connected => 0
		},
	);
	my $def_or_replace = sub {
		my ($hash, $key, $value) = @_;
		$hash->{$key} = $value unless defined $hash->{$key}
	};

	while (my ($key, $value) = each %defaults) {
		$def_or_replace->( $vm, $key, $value);
	}

	for my $nic (@{$vm->{nics}}) {
		$def_or_replace->($nic, "connected", 1);
		$def_or_replace->($nic, "type", "e1000");
	}

	for my $disk (@{$vm->{disks}}) {
		$def_or_replace->($disk, "thin", 1);
	}

	$def_or_replace->($vm->{cdrom},"connected",1);
}

# Add the MAC's from the created VM (@vmnics) to the configuration used to
# create the VM ($vm)
sub add_nic_macs {
	my ($vm, @vmnics) = @_;
	my $nics = $vm->{nics};

	die "Number of NICs in guest not the same as requested\n" unless @vmnics == @{$nics};
	for (my $i = 0; $i < @vmnics; $i++) {
		$nics->[$i]->{macaddress} = $vmnics[$i]->macAddress;
	}
}

# Check required values are provided.
sub check_config {
	my ($vm) = @_;
	my @missing = grep { not exists $vm->{$_} } qw(name computepath);
	if (@missing) {
	       	die "Missing required configuration items:\n".join("\n", @missing)."\n";
	}
}

# Load configuration from YAML
#my @vms = load_yaml( Opts::get_option('file') );
my @vms = load_config( Opts::get_option('file') );
#print Dumper(@vms);
#exit 1;
warn "Read VM configuration\n";
Util::connect();
warn "Connected to vCenter\n";

# Iterate over the VMs and create them
for my $vm (@vms) {
	warn "Creating VM\n";
	do_defaults($vm);
	check_config($vm);
	reset_unit_number();

	# 1. Find the datacenter
	warn "Finding datacenters\n";
	my $dc = get_dc( $vm->{datacenter} );
	unless ($dc) {
		warn "No datacenter's found! Skipping\n";
		next;
	}
	$vm->{datacenter} //= $dc->name;

	# 2. Get the compute resource
	warn "Finding cluster\n";
	my $host = get_host( $dc, [ "host", split qr#/#, $vm->{computepath} || "" ] );
	unless ($host) {
		warn "No such host ".$vm->{host}." for ".$vm->{name}."\n";
		next;
	}
	printf("Cluster: %s\n", $host->{name});

	my @datastores = @{ Vim::get_views(mo_ref_array => $host->datastore) };
#	foreach my $ds (@datastores) { printf("%s\n", $ds->name); }

	# 3. Find the datastore
	warn "Finding datastore\n";
	my $ds = do {
		my $tmp = get_ds_name(
			$host,
			$vm->{datastore},
		);
		unless ($tmp) {
			warn "Datastore not found: ".$vm->{datastore}."\n";
			next;
		}
		$vm->{datastore} = $tmp;
		"[$tmp]";
	};
	printf("Datastore: %s\n", $vm->{datastore});

	# Get coe_install name
	my $coe_install = get_coe_install($host);
	unless($coe_install) {
		printf("errror: unable to find coe_install datastore!\n");
		exit(1);
	}
	my $cdpath = $vm->{cdrom}->{path};
	$cdpath = "" unless($cdpath);
	$cdpath =~ s/\+COE\+/$coe_install/;
	$vm->{cdrom}->{path} = $cdpath;
	printf("cdpath: %s\n", $vm->{cdrom}->{path});

	# 4. Create devices
	my @devices = create_ctl( $vm );
	my $ide = create_ide_ctl;
	push @devices, create_ide_ctl;
	push @devices, create_disks( $vm, $ds );
	push @devices, create_nics( $vm, $host );
	push @devices, create_cdrom( $vm, $ide, $host );

	my $files = VirtualMachineFileInfo->new(
		logDirectory => undef,
		snapshotDirectory => undef,
		suspendDirectory => undef,
		vmPathName => $ds,
	);
	my $vm_config_spec = VirtualMachineConfigSpec->new(
		name => $vm->{name},
		version => $vm->{version},
		memoryMB => $vm->{memorySizeMB},
		files => $files,
		numCPUs => $vm->{numCpu},
		guestId => $vm->{guestId},
		deviceChange => \@devices,
		($vm->{annotation} ? (annotation => $vm->{annotation}) : ()),
	);

	# Find the folder to put the VM under
	warn "Finding folder\n";
	my $folder = get_vmfolder( $dc, [ "vm", split qr#/#, $vm->{folder} || "" ] );
	unless ($folder) {
		warn "Couldn't find a folder to put the VM in, skipping\n";
		next;
	}

	# Actually create the VM
	warn "Creating VM\n";
print Dumper($vm_config_spec);
exit(1);
	my $moref = $folder->CreateVM(
		config => $vm_config_spec,
		pool => $host->resourcePool
	);
#	warn $moref;

	# Find the NIC MACs
	my $new_vm = Vim::get_view(mo_ref => $moref);
#	my @nics = grep { $_->isa("VirtualEthernetCard") } @{ $new_vm->config->hardware->device };
#	add_nic_macs($vm, @nics);

	# Power on
	$new_vm->PowerOnVM() if ($vm->{poweron} eq "yes");
}
# Report the resulting VMs
#print Dumper(@vms);
Util::disconnect();
