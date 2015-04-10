#!/usr/bin/perl
#
# This is a script to check for symbol collisions in object files

# This is the main symbol map
%symbols;

# This is the object map, holding object file information
%object;

# This is the source map, holding source code line number information
%source;

# This is a mapping between the nm symbol types and readable names
%typemap = ( "A", "absolute symbol",
             "B", "uninitialized data",
             "D", "initialized data",
             "G", "initialized data",
             "R", "read-only data",
             "S", "uninitialized data",
             "T", "function"
           );

# rcg11102000 disable function clash output option.
$ignore_func_clash = 0;

# rcg11112000 disable type_info node clash output option.
$ignore_typeinfo_clash = 0;

# check command lines...
foreach(@ARGV) {
    if ($_ eq "--nofuncs") {
        $ignore_func_clash = 1;
    }

    if ($_ eq "--notypeinfo") {
        $ignore_typeinfo_clash = 1;
    }

    # other command line checks go here...
}


foreach(@ARGV) {
    $current_file = $_;

    # rcg11102000 skip "--params" ...
    if ($_ =~ /^--/) {
        next;
    }

    open(NM, "nm --no-sort --demangle --line-numbers $current_file |") ||
        die("Couldn't exec nm: $!\n");
    while (<NM>) {
        chop;
        /........ (.) ([^\/]+) *(\/*.*)/;
        $symbol = $2;
        $symbol_type = $1;
        $symbol_line = $3;
        $symbol =~ s/\s+$//;
        if ( $symbol eq "" ) {
            print "Line was: $_\n";
            next;
        }
        if ( $symbol_type =~ /[A-Z]/ &&
             $symbol_type ne "U" && $symbol_type ne "W" ) {
            if ( $symbols{$symbol} eq "" ) {
                $symbols{$symbol} = $symbol_type;
                $object{$symbol} = $current_file;
                $source{$symbol} = "$symbol_line";
	        } else {
                #if ( $location{$symbol} ne $symbol_line ) {
                if ( $object{$symbol} ne $current_file ) {
		            $last_type = $typemap{$symbols{$symbol}};
                    if ( $last_type eq "" ) {
                        $last_type = $symbols{$symbol};
                    }
                    $this_type = $typemap{$symbol_type};
                    if ( $this_type eq "" ) {
                        $this_type = $symbol_type;
                    }

                    #rcg11102000 optionally skip function symbol clashes.
                    # This is only if both symbols are functions; if you've
                    # got a function vs. data symbol clash, you've got big
                    # problems, so that'll still get printed.
                    if ($ignore_func_clash) {
                        if (($symbol_type eq "T") && ($last_type eq $this_type)) {
                            next;
                        }
                    }

		    #rcg11112000 optionally skip "type_info node" clashes.
		    # These are generally harmless, and sometimes unavoidable.
		    #  (i.e. intrinsic data types are always going to be there
		    #  but it's apparently no big deal.) If you don't need
		    #  Runtime Type Identification, compiling your code with
		    #  -fno-rtti will remove all these clashes, except the
		    #  ones for intrinsic data types.
		    if ($ignore_typeinfo_clash) {
			if ($symbol =~ /\btype_info/) {
                            next;
                        }
                    }
                    print <<__EOF__;
* Symbol $symbol multiply defined
First defined as $last_type in $object{$symbol}
 Source code: $source{$symbol}
Later defined as $this_type in $current_file
 Source code: $symbol_line

__EOF__
                }
            }
        }
    }
}
