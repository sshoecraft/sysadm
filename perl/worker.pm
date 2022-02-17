package worker;
use strict;
use warnings;
use threads;
use Data::Dumper;
use Time::HiRes qw(usleep);

use lib qw (/usr/local/lib/perl);
use utils;

$debug = 9;

my @cbs;

sub _worker {
	my $cb = shift;
	my $name = "worker" . $cb->{num};

	dprintf(3,"%s starting...\n", $name);
	do {
		dprintf(3,"%s busy: %s\n", $name, $cb->{busy});
		if ($cb->{busy}) {
			dprintf(3,"%s func: %p, args: %s\n", $name, $cb->{func}, join(' ',$cb->{args}));
			$cb->{func}($cb->{args});
		} else {
			usleep(200);
		}
	} while(!$cb->{done});
}

sub new {
	my ($class) = shift;

	my %options = ref $_[0] ? %{$_[0]} : @_;

	my $self = {};

	bless($self, $class);

	$self->{count} = $options{count} || '16';
	if ($options{func}) {
		$self->{func} = $options{func};
	} else {
		die 'func must be specified in constructor';
	}
	$self->{timeout} = $options{timeout} || '0';
	$self->{args} = $options{args} || undef;

	$self->cbs = [];
	$self->create($self->{count});

	dprintf(3,"timeout: %s, count: %s, args: %s\n", $self->{timeout}, $self->{count}, ($self->{args} ? $self->{args} : 'none'));

	return $self;
}

sub create {
	my $self = shift;
	my ($count) = shift;

	my $newcb;
	$newcb->{done} = 0;
	$newcb->{busy} = 0;
	$newcb->{func} = undef;
	$newcb->{args} = undef;

	$self->cbs[0] = 0;

	dprintf(3,"Creating threads...\n");
	for(my $i=0; $i < $count; $i++) {
		# Init the CB and create the threads
#		$self->cbs[0] = $newcb;
	#	threads->create(_worker, \$self->cbs[$i]);
	}
	$self->{count} = $count;
}

# Wait for all count to complete
sub wait {
	my $self = shift;

	dprintf(3,"waiting on threads...\n");

	# Tell each worker we're done
	foreach my $worker ($self->{workers}) {
		$worker->{done} = 1;
	}

	# Wait for them to finish
	foreach my $thr (threads->list) {
		if ($thr->tid && !threads::equal($thr, threads->self)) {
			$thr->join;
		}
        }
}

sub cleanup {
	my $self = shift;

	dprintf(3,"Cleaning up...\n");
	$self->cbs = [];
}

sub done {
	my $self = shift;

	$self->wait;
	$self->cleanup;
}
