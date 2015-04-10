#!/usr/bin/env perl

# build a pk3 for dedicated-only server from a file list
# suggested file list .. mp_*.pk3
# steps: 
#  scan for the .bsp and .script
#  extract them (following FS search order)
#  build pk3

# typical command line:
# ./extract-dedicated.pl /usr/local/games/wolfenstein/main/mp_*

# .bsp and .script are only part of the work anyway
# there are some other files needed
# but the architecture could be re-used to extract files in an easy way
# with a system similar to the FS

# get file list, alphabetical order
my @FILES = sort @ARGV;
print "FILES: @FILES\n";

my %TABLE;

# build a table, file name, where to extract from
for ($i = 0; $i < scalar(@ARGV); $i++)
{
  my $FILENAME = $FILES[$i];
  open( $log, 'unzip -l ' . "$FILENAME" . ' | egrep \'maps.*\.script\' | sed -e \'s/.*\(maps.*\)/\1/\' |');
  my $line = <$log>;
  do {
    chop $line;
    $TABLE{"$line"} = "$FILENAME";
  } until (!($line = <$log>));
  open( $log, 'unzip -l ' . "$FILENAME" . ' | egrep \'maps.*\.bsp\' | sed -e \'s/.*\(maps.*\)/\1/\' |');
  my $line = <$log>;
  do {
    chop $line;
    $TABLE{"$line"} = "$FILENAME";
  } until (!($line = <$log>));
}

system('rm -rf extract.tmp; mkdir extract.tmp');

my @FILELIST = keys(%TABLE);
for ($i = 0; $i < scalar(@FILELIST); $i++)
{
  print "File: $FILELIST[$i] $TABLE{$FILELIST[$i]}\n";
  system("unzip $TABLE{$FILELIST[$i]} $FILELIST[$i] -d extract.tmp");
}

