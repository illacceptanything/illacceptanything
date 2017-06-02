"use strict";
( function (root, factory) {
    if (typeof define === 'function' && define.amd) {
        define([ 'balalaika', 'matreshka_dir/polyfills/classlist', ], factory);
    } else {
        factory( root.$b );
    }
}(this, function ( $b ) {
	var s_classList = 'classList',
		_on, _off;
	if( !$b ) {
		throw new Error( 'Balalaika is missing' );
	}
	
	_on = $b.fn.on;
	_off = $b.fn.off;
	
	$b.extend( $b.fn, {
		on: function( n, f ) {
			n.split( /\s/ ).forEach( function( n ) {
				_on.call( this, n, f );
			}, this );
			return this;
		},
		off: function( n, f ) {
			n.split( /\s/ ).forEach( function( n ) {
				_off.call( this, n, f );
			}, this );
			return this;
		},
		hasClass: function( className ) { return !!this[ 0 ] && this[ 0 ][ s_classList ].contains( className ); },
		addClass: function( className ) {
			this.forEach( function( item ) {
				var classList = item[ s_classList ];
				classList.add.apply( classList, className.split( /\s/ ) );
			});
			return this;
		},
		removeClass: function( className ) {
			this.forEach( function( item ) {
				var classList = item[ s_classList ];
				classList.remove.apply( classList, className.split( /\s/ ) );
			});
			return this;
		},
		toggleClass: function( className, b ) {
			this.forEach( function( item ) {
				var classList = item[ s_classList ];
				if( typeof b !== 'boolean' ) {
					b = !classList.contains( className );
				}
				classList[ b ? 'add' : 'remove' ].apply( classList, className.split( /\s/ ) );
			});
			return this;
		},
		add: function( s ) {
			var result = $b( this ),
				ieIndexOf = function( a, e ) {
					for( var i = 0; i < a.length; i++ ) if( a[ i ] === e ) return i;
				};
			s = $b( s ).slice();
			[].push.apply( result, s );
			for( var i = result.length - s.length; i < result.length; i++ ) {
				if( ( [].indexOf ? result.indexOf( result[ i ] )  : ieIndexOf( result, result[ i ] ) ) !== i ) { // @IE8
					result.splice( i--, 1 );
				}
			}
			return result;
		},
		find: function( s ) {
			var result = $b();
			this.forEach( function( item ) {
				result = result.add( $b( s, item ) );
			});
			return result;
		}
	});
	
	// simple html parser
	$b.parseHTML = function( html ) {
		var node = document.createElement( 'div' ),
			// wrapMap is taken from jQuery
			wrapMap = {
					option: [ 1, "<select multiple='multiple'>", "</select>" ],
					legend: [ 1, "<fieldset>", "</fieldset>" ],
					thead: [ 1, "<table>", "</table>" ],
					tr: [ 2, "<table><tbody>", "</tbody></table>" ],
					td: [ 3, "<table><tbody><tr>", "</tr></tbody></table>" ],
					col: [ 2, "<table><tbody></tbody><colgroup>", "</colgroup></table>" ],
					area: [ 1, "<map>", "</map>" ],
					_: [ 0, "", "" ]
			},
			wrapper,
			i;
			
		html = html.replace( /^\s+|\s+$/g, '' );
		
		wrapMap.optgroup = wrapMap.option;
		wrapMap.tbody = wrapMap.tfoot = wrapMap.colgroup = wrapMap.caption = wrapMap.thead;
		wrapMap.th = wrapMap.td;
		
		wrapper = wrapMap[ /<([\w:]+)/.exec( html )[ 1 ] ] || wrapMap._;
		
		node.innerHTML = wrapper[ 1 ] + html + wrapper[ 2 ];
		
		i = wrapper[ 0 ];
		
		while( i-- ) {
			node = node.children[ 0 ];
		}
		
		return $b( node.children );
	};
	
	$b.create = function( tagName, props ) {
		var el = document.createElement( tagName ),
			i, j;
		if( props ) for( i in props ) {
			if( i == 'attributes' && typeof props[ i ] == 'object' ) {
				for( j in props[ i ] ) if( props[ i ].hasOwnProperty( j ) ) {
					el.setAttribute( j, props[ i ][ j ] );
				}
			} else  if( el[ i ] && typeof props == 'object' ) {
				el[ i ] = $b.extend( el[ i ] || {}, props[ i ] );
			} else {
				el[ i ] = props[ i ];
			}			
		}
		return el;
	};
	
	// @IE8 Balalaika fix. This browser doesn't support HTMLCollection and NodeList as second argument for .apply
	// This part of code will be removed in Matreshka 1.0
	(function( document, $, i, j, k, fn ) {
		var bugs,
			children = document.createElement( 'div' ).children;
		try { [].push.apply( [], children ); }
		catch( e ) { bugs = true; }
		bugs = bugs || typeof children === 'function' || document.documentMode < 9;

		if( bugs ) {
			fn = $.i[ j = 'prototype' ];

			$.i = function( s, context ) {
				k = !s ? fn : s && s.nodeType || s == window ? [s] : typeof s == 'string' ?  /</.test( s ) ? ( ( i = document.createElement( 'div' ) ).innerHTML = s, i.children ) : (context&&$(context)[0]||document).querySelectorAll(s) : /f/.test(typeof s) && (!s[0]&&!s[0].nodeType) ? /c/.test(document.readyState) ? s() : !function r(f){/in/(document.readyState)?setTimeout(r,9,f):f()}(s): s;
			
				j = []; for (i = k ? k.length : 0; i--; j[i] = k[i]) {}
				
				fn.push.apply( this, j );
			};
			
			$.i[ j ] = fn;
			
			fn.is = function( selector ) {
				var elem = this[ 0 ],
					elems = elem.parentNode.querySelectorAll( selector ),
					i;
				
				for ( i = 0; i < elems.length; i++ ) { if( elems[ i ] === elem ) return true; }
				return false;
			};
		}
		return $;
	})( document, $b );
	
	return $b;
}));
