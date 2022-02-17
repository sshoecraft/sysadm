#!/usr/bin/perl
# --------------------------------------------------------------------------------
# Script :   map.pl
# Revision : 1.1
# Author :   Rene Ribaud
# Description :
# --------------------------------------------------------------------------------
use strict;
use warnings;
#use diagnostics;   # Only use it for debugging purpose.

# --------------------------------------------------------------------------------
# Global variables
# --------------------------------------------------------------------------------
# Time
my ($sec,$min,$hour,$day,$month,$year,$wday,$yday,$isdst);
my $timestamp;

# Data
# Define a full structure
#my %mpaths =(
#        name =>{
#               size => undef,
#               uuid => undef,
#               dm => undef,
#               fs => undef,
#               disk =>         {
#                               diskname =>     {
#                                               port => undef,
#                                               lunid => undef,
#                                               sfnum => undef,
#                                               ldev => undef,
#                                               type => undef,
#                                               xp => undef,
#                                               },
#                               },
#               },
#        );

my %mpaths;

# --------------------------------------------------------------------------------
# Functions
# --------------------------------------------------------------------------------

# --------------------------------------------------------------------------------
# Main
# --------------------------------------------------------------------------------

# Get variable from command line

# Get timestamp
($sec,$min,$hour,$day,$month,$year,$wday,$yday,$isdst)=localtime();
$timestamp=sprintf("%4d%02d%02d-%02d:%02d:%02d",($year+1900),($month+1),$day,$hour,$min,$sec);

# Slurp tools data into arrays in memory
my @xpinfo=`xpinfo -i`;
my @multipath=`multipath -ll`;
my @mounts=`mount`;

for (my $line=0; $line<scalar(@multipath); $line++){
if ($multipath[$line] =~ /^mpath/){   # Check & parse line : mpath23 (360060e8015276a000001276a0000d0fc) dm-8 HP,OPEN-V
        (my $mpath, my $uuid, my $dm, my $type) = split ('\s+',$multipath[$line]);
        $uuid=~s/\(//g;
        $uuid=~s/\)//g;
        $mpaths{$mpath}->{"dm"}=$dm;
        $mpaths{$mpath}->{"uuid"}=$uuid;
        $mpaths{$mpath}->{"type"}=$type;
        # Crosscheck into mount to get the fs name
        # /dev/mapper/mpath14 on /oradata/crs/voting1 type ocfs2 (rw,_netdev,datavolume,nointr,heartbeat=local)
        my $fs;
        my @mount = grep(/$mpath\s/,@mounts);
        if (scalar(@mount==0)){
                $fs="N/A";
                }
                else
                {
                (undef,undef,$fs) = split('\s+',$mount[0]);
                }
        $mpaths{$mpath}->{"fs"}=$fs;

        # Next line to parse : [size=36G][features=1 queue_if_no_path][hwhandler=0][rw]
        $line++;
        if ($multipath[$line] =~ /^\[size=/){
                (my $size) = $multipath[$line] =~ /\[size=([0-9]+\w)/;
                $mpaths{$mpath}->{"size"}=$size;

                # Next line to parse : \_ round-robin 0 [prio=0][active]
                $line++;
                if ($multipath[$line] =~ /^\\_/){
                        # Next line to parse :  \_ 0:0:4:13 sdad 65:208 [active][ready]
                        $line++;
                        if ($multipath[$line] =~ /^ \\_/){
                                my $no_more_device=0;
                                do{
                                        (undef,undef,undef,my $disk, my $sf, undef) = split ('\s+',$multipath[$line]);
                                        $mpaths{$mpath}->{"disk"}->{$disk}->{"sfnum"}=$sf;
                                        # Crosscheck into xpinfo
                                        my @device = grep(/$disk\s/,@xpinfo);

                                        # Handle the case where device was not unmapped correctly
                                        # Unmapped from XP and not remoove from multipath.
                                        if (scalar(@device) == 0){
                                                $mpaths{$mpath}->{"disk"}->{$disk}->{"port"} = "Not found";
                                                $mpaths{$mpath}->{"disk"}->{$disk}->{"lunid"}= "N/A";
                                                $mpaths{$mpath}->{"disk"}->{$disk}->{"ldev"}= "N/A";
                                                $mpaths{$mpath}->{"disk"}->{$disk}->{"type"}= "N/A";
                                                $mpaths{$mpath}->{"disk"}->{$disk}->{"xp"}= "N/A";
                                        }
                                        else{
                                                #/dev/sdu                     76  04  02  CL2H  d0:f1  OPEN-V           00075626
                                                (undef,undef,undef, my $lunid, my $port, my $ldev, my $type, my $xp) = split('\s+',$device[0]);
                                                $mpaths{$mpath}->{"disk"}->{$disk}->{"port"}=$port;
                                                $mpaths{$mpath}->{"disk"}->{$disk}->{"lunid"}=$lunid;
                                                $mpaths{$mpath}->{"disk"}->{$disk}->{"ldev"}=$ldev;
                                                $mpaths{$mpath}->{"disk"}->{$disk}->{"type"}=$type;
                                                $mpaths{$mpath}->{"disk"}->{$disk}->{"xp"}=$xp;
                                        }
                                        if ((defined($multipath[$line+1])) && ($multipath[$line+1] =~ /^ \\_/)){
                                                $line++;
                                                }
                                                else {
                                                $no_more_device=1;
                                                }
                                        } while ($no_more_device==0);
                                }
                        }
                }
        }
}

# Print mapping
printf("%-34s %-9s %-7s %-6s %-6s %-8s %-6s %-6s %s\n","UUID","xp","type","size","dm","mapth","lunid","ldev","fs");
foreach my $mpath (sort(keys(%mpaths))){
        my @first_disk=keys(%{$mpaths{$mpath}->{"disk"}});
        # Check to report disk not unmapped correctly
        printf("%-34s %-9s %-7s %-6s %-6s %-8s %-6s %-6s %s\n"  ,$mpaths{$mpath}->{"uuid"}
                                                                ,$mpaths{$mpath}->{"disk"}->{$first_disk[0]}->{"xp"}
                                                                ,$mpaths{$mpath}->{"disk"}->{$first_disk[0]}->{"type"}
                                                                ,$mpaths{$mpath}->{"size"}
                                                                ,$mpaths{$mpath}->{"dm"}
                                                                ,$mpath
                                                                ,$mpaths{$mpath}->{"disk"}->{$first_disk[0]}->{"lunid"}
                                                                ,$mpaths{$mpath}->{"disk"}->{$first_disk[0]}->{"ldev"}
                                                                ,$mpaths{$mpath}->{"fs"}
                                                                );
        my $notfoundcount=0;
        foreach my $disk (keys(%{$mpaths{$mpath}->{"disk"}})){
                printf("%-6s %-8s %-4s\n",$disk,$mpaths{$mpath}->{"disk"}->{$disk}->{"sfnum"},$mpaths{$mpath}->{"disk"}->{$disk}->{"port"});

                if ($mpaths{$mpath}->{"disk"}->{$disk}->{"port"} eq "Not found"){
                        $notfoundcount++;
                        }

                if ($notfoundcount == scalar(keys(%{$mpaths{$mpath}->{"disk"}}))){
                        printf("This device was found in multipath but not reported by xpinfo.\n");
                        printf("Multipath device was probably not flushed after unmapping the device.\n");
                        printf("Try to run multipath -f %s.\n",$mpath);
                        }

                }

print "\n";
}
