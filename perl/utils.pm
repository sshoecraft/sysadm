#!/usr/bin/perl
package utils;
require Exporter;
@ISA = qw(Exporter);
@EXPORT = qw($debug dprintf _getdbdata getdbdata);
use strict;
use warnings;
use Data::Dumper;

our $debug = -1;

sub new {
	return bless {}, $_[0];
}

sub dprintf {
	my $level = shift;
	if ($debug >= $level) {
		my @args = @_;
		foreach(@args) {
			$_ = '' if (!defined($_));
			$_ =~ s/\%d/%s/g;
		}
#print Dumper(@args);
		printf(@args);
	}
}

sub get_cred($$) {
	return undef;
}

sub _getdbdata {
	my ($handle) = @_;

	dprintf(5,"_getrow: rows: %d\n", $handle->rows);
	if ($handle->rows < 1) {
		return undef;
	} else {
		return $handle->fetchrow_array();
	}
}

sub getdbdata {
	my ($handle,$arg) = @_;

	if (defined($arg)) {
		$handle->execute($arg);
	} else {
		$handle->execute();
	}
	my (@data) = _getdbdata($handle);
	print Dumper(@data) if ($debug >= 5);
	return @data;
}

sub getfqdn {
	use Net::DNS::Resolver;

	# Create resolver
	my $res = Net::DNS::Resolver->new;

	# Set searchlist
	# XXX dev.iknowmed.com has to be at the end because it's jacked up - like all of ikm
	$res->searchlist( 'uprd.usoncology.unx', 'ulab.usoncology.unx', 'uson.usoncology.int', 'usoncology.com', 'production.iknowmed.com', 'oc.mckesson.com', 'mckesson.com', 'dev.iknowmed.com', 'iknowmed.com');

	# Set server to localhost
	$res->nameservers( '127.0.0.1' );

	# Force V4?
	#$res->force_v4(1);

	dprintf(3,"getfqdn: arg: %s\n", $_[0]);
	my $query = $res->search($_[0]);
#	print Dumper($query);
	my $ans = $_[0];
	if ($query) {
		foreach my $rr ($query->answer) {
#			print Dumper ($rr);
#			next unless $rr->type eq "A";
#			printf("addr: %s\n",$rr->address);
			$ans = $rr->name;
			last;
		}
	}
	dprintf(3,"getfqdn: ans: %s\n", $ans);
	return $ans;
}

1;
