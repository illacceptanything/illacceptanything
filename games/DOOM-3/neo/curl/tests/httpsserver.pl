#!/usr/bin/perl
#
# $Id: httpsserver.pl,v 1.6 2003/10/29 16:27:43 bagder Exp $
# This is the HTTPS server designed for the curl test suite.
#
# It is actually just a layer that runs stunnel properly.

use strict;

my $stunnel = "stunnel";

#
# -p pemfile
# -P pid dir
# -d listen port
# -r target port
# -s stunnel path

my $verbose=0; # set to 1 for debugging

my $port = 8433; # just our default, weird enough
my $target_port = 8999; # test http-server port

my $path = `pwd`;
chomp $path;

my $srcdir=$path;

do {
    if($ARGV[0] eq "-v") {
        $verbose=1;
    }
    if($ARGV[0] eq "-w") {
        return 0; # return success, means we have stunnel working!
    }
    elsif($ARGV[0] eq "-r") {
        $target_port=$ARGV[1];
        shift @ARGV;
    }
    elsif($ARGV[0] eq "-s") {
        $stunnel=$ARGV[1];
        shift @ARGV;
    }
    elsif($ARGV[0] eq "-d") {
        $srcdir=$ARGV[1];
        shift @ARGV;
    }
    elsif($ARGV[0] =~ /^(\d+)$/) {
        $port = $1;
    }
} while(shift @ARGV);

my $conffile="$path/stunnel.conf";	# stunnel configuration data
my $certfile="$srcdir/stunnel.pem";	# stunnel server certificate
my $pidfile="$path/.https.pid";		# stunnel process pid file

open(CONF, ">$conffile") || return 1;
print CONF "
	CApath=$path
	cert = $certfile
	pid = $pidfile
	debug = 0
	output = /dev/null
	foreground = yes
	
	[curltest]
	accept = $port
	connect = $target_port
";
close CONF; 
#system("chmod go-rwx $conffile $certfile");	# secure permissions

		# works only with stunnel versions < 4.00
my $cmd="$stunnel -p $certfile -P $pidfile -d $port -r $target_port 2>/dev/null";

# use some heuristics to determine stunnel version
my $version_ge_4=system("$stunnel -V 2>&1|grep '^stunnel.* on '>/dev/null 2>&1");
		# works only with stunnel versions >= 4.00
if ($version_ge_4) { $cmd="$stunnel $conffile"; }

if($verbose) {
    print "HTTPS server: $cmd\n";
}

system($cmd);

unlink $conffile;
