"use strict";
(function (root, factory) {
    if (typeof define === 'function' && define.amd) {
        define(['matreshka_dir/balalaika-extended'], factory);
    } else {
        root.__DOLLAR_LIB = factory( root.$b );
    }
}(this, function ( $b ) {
	var neededMethods = 'on off is hasClass addClass removeClass toggleClass add find'.split( /\s+/ ),
		dollar = typeof $ == 'function' ? $ : null,
		useDollar = true,
		i;
	
	if( dollar ) {
		for( i = 0; i < neededMethods.length; i++ ) {
			if( !dollar.prototype[ neededMethods[ i ] ] ) {
				useDollar = false;
				break;
			}
		}
		
		if( !dollar.parseHTML ) {
			useDollar = false;
		}
	} else {
		useDollar = false;
	}
	
    return useDollar ? dollar : $b;
}));