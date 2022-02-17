#!/usr/bin/perl
##############################################################################################
#
# File:         replay.pl
# Description:  Replay a client's replay file against System Designer
# Author:       Lee Mayes   ( email leem@hp.com )
# Created:      31 May 2007
# Language:     perl
# Package:      LinuxCOE
#
##############################################################################################
# © Copyright 2000-2009 Hewlett-Packard Development Company, L.P
#
# This program is free software; you can redistribute it and/or modify it under the terms of
# the GNU General Public License as published by the Free Software Foundation; either version
# 2 of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
# without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with this program;
# if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
##############################################################################################
# Basic process flow
# Step 1 - Process Args
# Step 2 - Determine Backend
# Step 3 - Construct URL
# Step 4 - Contact SysDes to create image
# Step 5 - Download image
# Step 6 - Output local filename and exit
# NOTE: Any non 0 exit value indicates an error occurred!
##############################################################################################

&Usage unless (@ARGV);

use strict;
use CGI;
use LWP::UserAgent;
use HTTP::Request::Common qw(POST);
use Socket;

my $debug = 1;								# Debug info on STDERR?

# My defaults - MODIFY TO MATCH YOUR SITE!!!!!!
my $sysdes = 'http://localhost';				# Your instance (port # optional)
my $cgi_path = 'systemdesigner-cgi-bin';			# Your cgi path

# Snag the values passed, only the first 2 interest me for now
my ($filename,$def,@rest) = @ARGV;

# Open replay file
unless ( open(REPLAY,"$filename")) {
  my $err = "Cannot open replay file ($filename) for reading : $!\n";
  print STDERR $err if $debug;
  print STDOUT $err;
  exit 2;
}
print STDERR "Opened $filename for replay\n" if $debug;

# Build a list of all the parms
my ($buf,$parms,$dist,$way,$real_way);
while($buf=<REPLAY>) {
 next if ( $buf =~ /^#/ );		# no comment
 chomp($buf);
 next unless ($buf);			# no blanks
#print "buf: $buf\n";
 $parms .= "&$buf";
 if ( $buf =~ /^real_way=/ ) {
	$buf =~ s/^real_way=//;
	$real_way = $buf;
 }
 if ( $buf =~ /^waystation=/ ) {
	$buf =~ s/^waystation=//;
	$way = $buf;
 }
 if ( $buf =~ /^os=/ ) {		# Snag distro to determine which backend to call
   $buf =~ s/^os=//;
   ($dist,@rest) = split('%',$buf);  	
 } elsif ( $buf =~ /Pass/ ) {		# It's a password, dupe it so backend thinks it's 'verified'
   $parms .= "&$buf";
 }
}
close(REPLAY);
$real_way = "" unless($real_way);
$way = "" unless($way);
if (length($real_way) > 0) {
	$sysdes = "http://" . $real_way;
} else {
	$sysdes = "http://" . $way;
}

# Backend varies based on distro, call coe_info to get correct one
my $backend = &which_backend($dist);

# Construct the URL to call SysDes with
my $sysdes_url = "${sysdes}/${cgi_path}/${backend}";  # What I'll call
print STDERR "BaseURL : $sysdes_url\n" if $debug;

# Append the rest....
$parms = 'action=replay' . $parms;
if ( $def ) {  $parms .= "&defs=$def" }
print STDERR "FinalURL : $sysdes_url $parms\n" if $debug;

# Create the image
my $img_url = &et_phone_home($sysdes_url,$parms);

my @data = split(':::',$img_url);
foreach my $buf (shift(@data)) {
  if ($buf =~ /^ERR/) {
    print STDERR "$img_url\n" if $debug;
    print "$img_url\n";
    exit 4;
  }
  next unless ( $buf =~ /START_OF_LINUXCOE_BEAM_DATA/);
}
$img_url = shift(@data);
my ($method,$rest) = split('//',$img_url);
my @paths = split('/',$rest);
my $host_name = $paths[0];
printf("host_name: %s\n", $host_name);
#my $host;
#my $packed_ip  = gethostbyname($host_name);
#if (defined $packed_ip) {
#	$host = inet_ntoa($packed_ip);
#} else {
#	$host = $host_name;
#}
#$host = $host_name if (!length($host));
my $iso = $paths[(@paths)-1];
#my $img_url = $method . "//" . $host . "/sysdes/" . $iso;
my $img_url = $method . "//" . $way . "/sysdes/" . $iso;
print "URL=$img_url\n";
#print STDERR "Will fetch $img_url\n" if $debug;
exit 0;

(@data) = split('/',$img_url);
$filename = pop(@data);
unlink $filename if ( -r "$filename" );
if ( -r "$filename" ) {
  my $err = "ERR: I can't remove $filename, get it out of my way and retry please.\n";
  print STDERR $err if $debug;
  print $err;
  exit 5;
}
#system "wget -q $img_url";
print "$filename\n";
exit 0;					# It's all good


#### END of MAIN

sub which_backend {

# Find out which back-end to call based on distro being mangled
 
  my $dist = shift(@_);
  my $url = "${sysdes}/${cgi_path}/coe_info/backend/$dist";
  my $result = &et_phone_home($url);
  if ( $result =~ /^ERR/ ) {
    print STDERR $result if $debug;
    print STDOUT $result;
    exit 3;
  } elsif ( $result =~ /NONE/ ) {
    $result = "ERR: Sorry, I don't currently support distro $dist, file an enhancement request.\n";
    print STDERR $result if $debug;
    print STDOUT $result;
    exit 6;
  }
  return($result);
  
}

sub et_phone_home {

# Contact the SystemDesigner instance for data

  my ($action,$parms) = @_;
  my $ua = LWP::UserAgent->new;
  $ua->agent("LinuxCOE-replay/4.0 ");
  print STDERR "Phoning home with ${action}\n" if $debug;
  my $req = HTTP::Request->new(POST => $action);
  $req->content_type('application/x-www-form-urlencoded');
  $req->content($parms);
  my $results;
  my $res = $ua->request($req);
  if ($res->is_success) {
    my @data = split('\n',$res->content);
#    foreach(@data) { printf("data: %s\n",$_); }
    while ($buf = shift(@data)) {
      last if ($buf eq 'START_OF_LINUXCOE_BEAM_DATA');
    }
    while ($buf = pop(@data)) {
      last if ($buf eq 'END_OF_LINUXCOE_BEAM_DATA');
    }
    $results = join(':::',@data);
  } else {
    $results = "ERR: " . $res->status_line ."\n";
  }
  print STDERR "ET returning $results\n" if $debug;
  return($results);

}

sub Usage {

  print qq{
Usage: replay.pl replayfile [ defs ]

replayfile - absolute or relative path to a valid replay file
defs       - optional additional default attribute to read (?defs=XXX on URLs)

Ex:  replay.pl my_replay_file SIM

};
  print STDERR "See usage dude(tte)!\n" if $debug;
  exit 1;

}

