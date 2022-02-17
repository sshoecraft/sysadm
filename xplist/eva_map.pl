#!/usr/bin/perl

use strict;
use warnings;
use Data::Dumper;

my @evainfo=`evainfo -a`;
my @ioscan=`ioscan -m dsf`;
my %devmap;
my %devmap2;
my %lunmap;
my %sf;

my $dsf;
my $olddsf;
my $legacy;

# Create map : "legacy device" = "dsf device"
foreach(@ioscan){
        if ( /\/dev/ ){
                ($dsf, $legacy)=split(/\s+/,$_);
                if ( $dsf eq "" ){
                        $dsf=$olddsf;
                }
                $devmap{$legacy}=$dsf;
        $olddsf=$dsf;
        }
}

# Create map : "wwnn id" = "legacy device"
foreach(@evainfo){

        if ( /\/dev/){
                (my $legacy, undef, my $wwnn, my $size, my $ctrl)=split(/\s+/,$_);
                $lunmap{$wwnn}=$legacy;
        }
}

# Create map : "dsf device" = "major and minor"
%devmap2=%devmap; # values() modify the hash, so copy to a new one to avoid %devmap to be modified
foreach(values(%devmap2)){
        open (SHELLCMD,"ll $_ |");
                while (<SHELLCMD>){
                        (undef,undef,undef,undef,my $major, my $minor,undef,undef,undef,my $dev)=split(/\s+/,$_);
                        $sf{$dev}->{"major"}=$major;
                        $sf{$dev}->{"minor"}=$minor;
                }
        close (SHELLCMD)
}

# Debug purpose
#print Dumper(\%devmap);
#print Dumper(\%lunmap);
#print Dumper(\%sf);

# Print output
foreach(sort(keys(%lunmap))){
        my $legacy=$lunmap{$_};
        printf("%s %s %s %s\n",$_ ,$devmap{$legacy}, $sf{$devmap{$legacy}}->{"major"}, $sf{$devmap{$legacy}}->{"minor"});
}
