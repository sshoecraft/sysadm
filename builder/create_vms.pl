#!/usr/bin/perl
use strict;
use warnings;
use Data::Dumper;
use XML::Simple;
use VMware::VIRuntime;
use feature "switch";

my $debug = 0;
sub dprintf { my $level = shift; if ($debug >= $level) { print "#"; printf(@_); } }

$Util::script_version = "1.0";

my %opts = (
	file => {
		type => "=s",
		help => "config file path",
		required => 0,
	},
	opts => {
		type => "=s",
		help => "advanced options",
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
#			print Dumper($child);
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

# Find the cluster/folder
sub get_cluster {
	my ($dc, $name) = @_;

	dprintf(3,"get_cluster: dc: %s, name: %s\n", $dc->name, $name);
	my $cluster;

#	if ($name =~ m:/:) {
		$cluster = find_folder( Vim::get_view(mo_ref => $dc->hostFolder), [],
			[ "host", split qr#/#, $name || "" ] );
#	} else {
	if (0) {
		my (@clusters) = Vim::find_entity_views(view_type => "ClusterComputeResource", filter => { 'name' => $name });
		for my $c1 (@clusters) {
			for my $c2 (@$c1) {
				dprintf(3,"name: %s, parent: %s, dc: %s\n", $c2->name, $c2->parent->value, $dc->hostFolder->value);
				if ($c2->parent->value eq $dc->hostFolder->value) {
					dprintf(3,"get_cluster: match");
					$cluster = $c2;
					last;
				}
			}
		}
	}
	unless($cluster) {
		printf("error: cluster not found!\n");
		exit(1);
	}
#	print Dumper($cluster);
	return $cluster;
}

# Find a compute resource at $path
sub old_get_cluster {
	my ($dc, $path) = @_;

	dprintf(1,"get_cluster: dc: %s, path: %s\n", $dc, @$path);
	for my $p (@$path) { printf("p: %s\n", $p); }
	if ($path) {
		return find_folder( Vim::get_view(mo_ref => $dc->hostFolder), [], $path);
	}
	# Pick one at random
	else {
		die "error, no cluster or folder specified!\n";
	}
}

# Get a host from a cluster/folder
sub get_host {
	my ($cluster) = @_;

	printf("ref: %s\n", ref $cluster);
	dprintf(1,"get_host: cluster: %s\n", $cluster->name);

	my $host;
	my $list = ($cluster->{host} ? $cluster->{host} : $cluster->{childEntity});
	for my $host_ref (@{$list}) {
		$host = Vim::get_view(mo_ref => $host_ref);
		last;
	}
	dprintf(1,"get_host: returning: %s\n", $host->name) if ($host);
	return $host;
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

sub get_coe_images {
	my ($host) = @_;

	my @datastores = @{ Vim::get_views(mo_ref_array => $host->datastore) };
#	foreach my $ds (@datastores) { printf("ds: %s\n", $ds->{name}); }
	my ($target) = grep { $_->summary->name =~ /coe-images/} @datastores;
#	printf("type: %s\n", ref($target));
	if ($target) {
#		printf("name: %s\n", $target->summary->name);
		return $target->summary->name;
	}
#	foreach my $ds (@datastores) { printf("ds: %s\n", $ds->{name}); }
}

# Check the datastore exists on the host, if no datastore is given, return the
# store with the greatest free space
sub get_ds_name {
	my ($host, $store) = @_;

#	dprintf(1,"host->name: %s, store: %s\n", $host->{name}, $store );
#	die "cluster has no datastores!" unless($host->{datastore});
	my @datastores = @{ Vim::get_views(mo_ref_array => $host->{datastore})};
	my $target;
	# Remove COE stores from list
	@datastores = grep { $_->summary->name !~ /coe/ } @datastores;
#	print Dumper(@datastores);
#	exit(1);
	@datastores = grep { $_->summary->maintenanceMode !~ /inMaintenance/ } @datastores;
#	foreach my $ds (@datastores) { printf("%s\n", $ds->name); }
	if ($store) {
		($target) = grep { $_->summary->name eq $store } @datastores;
	} else {
		($target) = sort { $b->summary->freeSpace <=> $a->summary->freeSpace } @datastores;
	}
	if ($target) {
		if ($target->summary) {
			dprintf(1,"name: %s\n", $target->summary->name);
			my $name = $target->summary->name;
			return "[$name]";
		}
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
	# XXX If any NIC type is vmxnet3, set controller to paravirt
	my $do_paravirt = 0;
	for my $nic (@{$vm->{nics}}) {
		if ($nic->{type} eq "vmxnet3") {
			$do_paravirt = 1;
			last;
		}
	}
	if ($do_paravirt) {
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
		dprintf(3,"size: %s\n", $disk->{size});
		next if ($disk->{size} eq "0");
		dprintf(3,"continuing...\n");
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

sub find_network {
	my ($ref_networks, $name) = @_;

	my @networks = sort @{$ref_networks};
	dprintf(1,"find_network: name: %s\n", $name);

	my $net_name;
	for my $net (@networks) {
		dprintf(3,"find_network: net->summary->name: %s\n", $net->{summary}->{name});
		if ($net->{summary}->{name} eq $name) {
			$net_name = $net->{summary}->{name};
			last;
		}
	}
	unless($net_name) {
		for my $net (@networks) {
			dprintf(3,"find_network: net->summary->name: %s\n", $net->{summary}->{name});
			if ($net->{summary}->{name} =~ m/$name/i) {
				$net_name = $net->{summary}->{name};
				last;
			}
		}
	}
	dprintf(1,"find_network: found: %s\n", $net_name) if ($net_name);
	return $net_name;
}

# Create the NIC configuration
sub create_nics {
	my ($vm, $host, $dc) = @_;
#	my %networks = map { $_->name => $_ } @{ Vim::get_views(mo_ref_array => $host->network) };
	my @host_networks = @{ Vim::get_views(mo_ref_array => $host->network) };
	my %networks = map { $_->name => $_ } @host_networks;

#	my $net_view = Vim::get_views(mo_ref_array => $host->network);

	my @nics;
	for my $nic (@{$vm->{nics}}) {
		my $net_name = find_network(\@host_networks,$dc . "-" . $nic->{network});
		$net_name = find_network(\@host_networks,$nic->{network}) unless($net_name);
		$net_name = find_network(\@host_networks,"VM_Network") unless($net_name);
		$net_name = "" unless($net_name);

		# TODO: Exit more cleanly
		exists $networks{ $net_name } || die "unable to find network: ".$nic->{network}."\n";

		my $network = $networks{ $net_name };

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
				deviceName => $net_name,
				network => $network,
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

sub wait_task($) {
	my ($taskRef) = @_;

	my $task_view = Vim::get_view(mo_ref => $taskRef);
	while(1) {
        	my $val = $task_view->info->state->val;
#		print Dumper($val);
		last if ($val eq "success" || $val eq "error");
		sleep 1;
		$task_view->ViewBase::update_view_data();
	}
}

sub getStatus {
        my ($taskRef,$message) = @_;

        my $task_view = Vim::get_view(mo_ref => $taskRef);
        my $taskinfo = $task_view->info->state->val;
        my $continue = 1;
        while ($continue) {
                my $info = $task_view->info;
                if ($info->state->val eq 'success') {
                        print $message,"\n";
                        return $info->result;
                        $continue = 0;
                } elsif ($info->state->val eq 'error') {
                        my $soap_fault = SoapFault->new;
                        $soap_fault->name($info->error->fault);
                        $soap_fault->detail($info->error->fault);
                        $soap_fault->fault_string($info->error->localizedMessage);
                        die "$soap_fault\n";
                }
                sleep 5;
                $task_view->ViewBase::update_view_data();
        }
}

# Turn the version into a value
sub verval {
	my ($vstr) = @_;
	my $val = 0;

	dprintf(3,"verval: vstr: %s\n", $vstr);
	my @vals = split('\.',$vstr);
	foreach my $d (@vals) {
		$val <<= 8;
		$d = 0 unless($d =~ /^[+-]?\d+$/);
		$val |= $d;
		dprintf(5,"verval: val: %x, item: %d\n", $val, $d);
	}
	dprintf(3,"verval: returning: %d\n", $val);
	return $val;
}

# Load configuration from YAML
#my @vms = load_yaml( Opts::get_option('file') );
my @vms = load_config( Opts::get_option('file') );
#print Dumper(@vms);
#exit 1;
#warn "Read VM configuration\n";

# Get user and pass
my $server = Opts::get_option('server');
my $user = `cat /usr/local/etc/.vim  | grep "^$server" | head -1 | awk '{ print \$2 }'`;
chomp($user);
$user = "" unless($user);
if (!length($user)) {
        printf("error: unable to find user for server, aborting\n");
        exit(1);
}
my $pass = `/usr/local/bin/vim_cred -g -x -s '$server' -u '$user'`;
chomp($pass);
dprintf(3,"user: %s, pass: %s\n", $user, $pass);

dprintf(1,"Connecting to server...\n");
$ENV{'VI_PROTOCOL'} = 'https';
$ENV{'VI_SERVER'} = $server;
$ENV{'VI_SERVICEPATH'} = '/sdk';
$ENV{'VI_USERNAME'} = $user;
$ENV{'VI_PASSWORD'} = $pass;
Opts::parse();
#Opts::validate();
dprintf(3,"Connecting...\n");
Util::connect();
warn "Connected to vCenter\n";

#my $vmSpec = VirtualMachineConfigSpec->new(numCPUs => 1);
#$vmSpec->{guestId} = "vlah";
#print Dumper($vmSpec);
#exit(9);


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
		warn "unable find datacenter: " . $vm->{datacenter} . "\n";
		next;
	}
	dprintf(1,"dc->name: %s\n", $dc->name);
	$vm->{datacenter} //= $dc->name;

	# 2. Get the compute resource / folder
	warn "Finding cluster\n";
	dprintf(1,"computepath: %s\n", $vm->{computepath});
#	my $cluster = get_cluster( $dc, [ "host", split qr#/#, $vm->{computepath} || "" ] );
	my $cluster = get_cluster( $dc, $vm->{computepath} );
	unless ($cluster) {
		warn "unable to find compute resource at path: " . $vm->{computepath} . "\n";
		next;
	}
	dprintf(1,"Cluster: %s\n", $cluster->{name});

	# 3. Get the host if a Folder
	my $host;
	if (ref $cluster eq "Folder") {
		warn "Getting host\n";
		$host = get_host($cluster);
	} else {
		$host = $cluster;
	}

#	my @datastores = @{ Vim::get_views(mo_ref_array => $host->datastore) };
#	foreach my $ds (@datastores) { printf("%s\n", $ds->name); }

	# 3. Find the datastore
	warn "Finding datastore\n";
if (0) {
	my $ds = do {
		my $tmp = get_ds_name(
			$host,
			$vm->{datastore},
		);
		unless ($tmp) {
			warn "Datastore not found: ".$vm->{datastore}."\n" if ($vm->{datastore});
			next;
		}
		printf("tmp: %s\n", $tmp);
		$vm->{datastore} = $tmp;
		"[$tmp]";
	};
}
	my $ds = get_ds_name($host,$vm->{datastore});
	unless($ds) {
		if ($vm->{datastore}) {
			printf("error: datastore not found: %s\n",$vm->{datastore});
		} else {
			printf("error: unable to select datastore!\n");
		}
		exit(1);
	}
	printf("Selected datastore: %s\n", $ds);

	# Get coe-images datastore name
	my $coe_images = get_coe_images($host);
	unless($coe_images) {
		printf("errror: unable to find coe_images datastore!\n");
		exit(1);
	}
	my $cdpath = $vm->{cdrom}->{path};
	$cdpath = "" unless($cdpath);
	$cdpath =~ s/\+COE\+/$coe_images/;
	$vm->{cdrom}->{path} = $cdpath;
	printf("cdpath: %s\n", $vm->{cdrom}->{path});

	# 4. Create devices
	my @devices = create_ctl( $vm );
	my $ide = create_ide_ctl;
	push @devices, create_ide_ctl;
	push @devices, create_disks( $vm, $ds );
	push @devices, create_nics( $vm, $host, $dc->name );
	push @devices, create_cdrom( $vm, $ide, $host );

	my $files = VirtualMachineFileInfo->new(
		logDirectory => undef,
		snapshotDirectory => undef,
		suspendDirectory => undef,
		vmPathName => $ds,
	);

#		($vm->{annotation} ? (annotation => $vm->{annotation}) : ()),
#		guestId => $vm->{guestId},
#		extraConfig => \@opts,
	my $vm_config_spec = VirtualMachineConfigSpec->new(
		name => $vm->{name},
		version => $vm->{version},
		memoryMB => $vm->{memorySizeMB},
		numCPUs => $vm->{numCpu},
		files => $files,
		deviceChange => \@devices,
	);
#	print Dumper($vm_config_spec);

	$vm->{annotation} = "" unless($vm->{annotation});
	$vm->{annotation} = "" if (ref $vm->{annotation} eq "HASH");
	if (length($vm->{annotation}) > 0) {
		warn "Setting Annotation: $vm->{annotation}\n";
		$vm_config_spec->{annotation} = $vm->{annotation};
	}

	# Find the folder to put the VM under
	warn "Finding folder\n";
	my $folder = get_vmfolder( $dc, [ "vm", split qr#/#, $vm->{folder} || "" ] );
	unless ($folder) {
		warn "Couldn't find a folder to put the VM in, skipping\n";
		next;
	}
#	dprintf(1,"vm->{folder}: %s\n", $vm->{folder}) if vm->{folder};

	# Actually create the VM
	warn "Creating VM\n";
#	print Dumper($vm_config_spec);
	my $moref = $folder->CreateVM( config => $vm_config_spec, pool => $host->resourcePool );

	# Get view of new VM
	dprintf(3,"Getting view\n");
	my $new_vm = Vim::get_view(mo_ref => $moref);
#	my $new_vm = Vim::find_entity_view(view_type => 'VirtualMachine',filter => { 'name' => $vm->{name} });
#	print Dumper($new_vm);
#	$host = Vim::get_view(mo_ref => $new_vm->{host});
	dprintf(3,"Getting host view...\n");
	$host = Vim::get_view(mo_ref => $new_vm->summary->runtime->host);
#	print Dumper($host);
	dprintf(2,"host apiVersion: %s \n", $host->config->product->apiVersion);
	my $host_verval = verval($host->config->product->apiVersion);
	dprintf(2,"host verval: %s\n", $host_verval);

	# Find the NIC MACs (XXX why again?)
#	my @nics = grep { $_->isa("VirtualEthernetCard") } @{ $new_vm->config->hardware->device };
#	add_nic_macs($vme, @nics);

	# Upgrade to latest supported
	warn "Upgrading VM HW\n";
	eval { $new_vm->UpgradeVM() };

	# Create a new spec for possible changes
	my $vmSpec = VirtualMachineConfigSpec->new;
	my $changed = 0;

	# Set guestID
	if ($new_vm->config->{guestId} ne $vm->{guestId}) {
		$vmSpec->{guestId} = $vm->{guestId};
		$changed++;
	}

	dprintf(2,"guestId: %s, profile: %s\n", $vmSpec->{guestId}, $vm->{profile});
	my $savedId = $vm->{guestId};
	if ($host_verval >= verval('6.7')) {
		if ($vm->{profile} eq "c8-64") {
			$vmSpec->{guestId} = "centos8_64Guest";
		} elsif ($vm->{profile} eq "ol8-64") {
			$vmSpec->{guestId} = "oracleLinux8_64Guest";
		} elsif ($vm->{profile} eq "rh8-64") {
			$vmSpec->{guestId} = "rhel8_64Guest";
		}
	}
	if ($host_verval >= verval('6.5')) {
		if ($vm->{profile} eq "c6-32") {
			$vmSpec->{guestId} = "centos6Guest";
		} elsif ($vm->{profile} eq "c6-64") {
			$vmSpec->{guestId} = "centos6_64Guest";
		} elsif ($vm->{profile} eq "c7-64") {
			$vmSpec->{guestId} = "centos7_64Guest";
		} elsif ($vm->{profile} eq "ol6-32") {
			$vmSpec->{guestId} = "oracleLinux6Guest";
		} elsif ($vm->{profile} eq "ol6-64") {
			$vmSpec->{guestId} = "oracleLinux6_64Guest";
		} elsif ($vm->{profile} eq "ol7-64") {
			$vmSpec->{guestId} = "oracleLinux7_64Guest";
		}
	}
	if ($host_verval >= verval('5.5')) {
		if ($vm->{profile} eq "rh5-32") {
			$vmSpec->{guestId} = "rhel5Guest";
		} elsif ($vm->{profile} eq "rh5-64") {
			$vmSpec->{guestId} = "rhel5_64Guest";
		} elsif ($vm->{profile} eq "rh6-32") {
			$vmSpec->{guestId} = "rhel6Guest";
		} elsif ($vm->{profile} eq "rh6-64") {
			$vmSpec->{guestId} = "rhel6_64Guest";
		} elsif ($vm->{profile} eq "rh7-64") {
			$vmSpec->{guestId} = "rhel7_64Guest";
		}
	}
	dprintf(2,"guestId: %s, savedId: %s\n", $vmSpec->{guestId}, $savedId);
	if (defined($vmSpec->{guestId}) && $vmSpec->{guestId} ne $savedId) {
		dprintf(2,"NEW guestId: %s\n", $vmSpec->{guestId});
		$changed++;
	}
# XXX
#	printf("warning: %s is not supported on ESX version %s\n", $vm->{profile}, $host->config->product->apiVersion);

	dprintf(1,"numCPU: %s, numCoresPerSocket: %s\n", $new_vm->config->{hardware}->{numCPU},
		$new_vm->config->{hardware}->{numCoresPerSocket});
	if ($new_vm->config->{hardware}->{numCoresPerSocket} != $new_vm->config->{hardware}->{numCPU}) {
		$vmSpec->{numCoresPerSocket} = $new_vm->config->{hardware}->{numCPU};
		$changed++;
	}

	if (0) {
		dprintf(1,"Setting tools upgrade policy...\n");
		my $toolsConfig = ToolsConfigInfo->new(
			afterPowerOn => 'true',
			afterResume => 'true',
			beforeGuestStandby => 'true',
			beforeGuestShutdown => 'true',
			toolsUpgradePolicy => 'upgradeAtPowerCycle',
			syncTimeWithHost => 'false',
		);
		$vmSpec->tools = $toolsConfig;
		$changed++;
	}

	# Set options
	my $optstr = Opts::get_option('opts');
	$optstr = "" unless($optstr);
#	print Dumper($optstr);
	my @opts = ();
	my $nested_hv = 0;
	foreach(split('\|',$optstr)) {
#		printf("opt: %s\n", $_);
		my @tmp = split('=',$_);
		if ($tmp[0] eq 'vhv.enable' && $tmp[1] eq 'TRUE') {
			$nested_hv = 1;
		} else {
			my $val = OptionValue->new(key => $tmp[0], value => $tmp[1]);
#			push @opts, $val;
		}
	}
	if (scalar @opts) {
		$vmSpec->extraConfig = \@opts;
		$changed++;
	}

	# Set nested
	if ($nested_hv) {
		$vmSpec->{nestedHVEnabled} = 1;
		$changed++;
	}

	# Reconfig
	if ($changed) {
		warn "Reconfiguring...\n";
#		print Dumper($vmSpec);
 		my $task = $new_vm->ReconfigVM_Task(spec => $vmSpec);
		wait_task($task);
	}

	# IF < 9 vcpu, enable hot add
	if ($new_vm->config->{hardware}->{numCPU} < 9) {
		if (!$new_vm->config->hardware->{cpuHotAddEnabled}) {
			dprintf(1,"Enabling CPU hotplug...\n");
			my $spec = VirtualMachineConfigSpec->new(cpuHotAddEnabled => 'true');
			eval {
				my $task = $new_vm->ReconfigVM_Task(spec => $spec);
#				getStatus($task,$new_vm->{name} . ": CPU reconfig successful");
				wait_task($task);
			};
		}
	}

	if (!$new_vm->config->hardware->{memoryHotAddEnabled}) {
		dprintf(1,"Enabling Memory hotplug...\n");
		eval {
			my $spec = VirtualMachineConfigSpec->new(memoryHotAddEnabled => 'true');
			my $task = $new_vm->ReconfigVM_Task(spec => $spec);
#			getStatus($task,$new_vm->{name} . ": Memory reconfig successful");
			wait_task($task);
		};
	}

	# Power on
	if ($vm->{poweron} eq "yes") {
		warn "Powering on\n";
		$new_vm->PowerOnVM();
	}
}
# Report the resulting VMs
#print Dumper(@vms);
Util::disconnect();
