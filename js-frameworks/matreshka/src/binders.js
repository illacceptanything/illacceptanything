"use strict";
(function (root, factory) {
    if (typeof define === 'function' && define.amd) {
        define( factory );
    } else {
        root.__MK_BINDERS = factory();
    }
}(this, function ( MK ) {
	var oneWayBinder = function( f ) {
		return { on: null, getValue: null, setValue: f };
	},
	binders;
	
	return binders = {
		innerHTML: function() {// @IE8
			return oneWayBinder( function( v ) {
				this.innerHTML = v === null ? '' : v + '';
			});
		},
		className: function( className ) {
			var not = !className.indexOf( '!' );
			if( not ) {
				className = className.replace( '!', '' );
			}
			return oneWayBinder( function( v ) {
				this.classList.toggle( className, not ? !v : !!v );
			});
		},
		property: function( propertyName ) {
			return oneWayBinder( function( v ) {
				this[ propertyName ] = v;
			});
		},
		attribute: function( attributeName ) {
			return oneWayBinder( function( v ) {
				this.setAttribute( attributeName, v );
			});
		},
		textarea: function() {
			return binders.input( 'text' );
		},
		input: function( type ) {
			var on;
			switch( type ) {
				case 'checkbox': 
					return {
						on: 'click keyup',
						getValue: function() { return this.checked; },
						setValue: function( v ) { this.checked = v; }
					};
				case 'radio':
					return {
						on: 'click keyup',
						getValue: function() { return this.value; },
						setValue: function( v ) {
							this.checked = this.value == v;
						}
					};
				case 'submit':
				case 'button':
				case 'image':
				case 'reset':
					return {};
				case 'hidden':
					on = '';
					break;
				case 'text':
				case 'email':
				case 'password':
				case 'tel':
				case 'url':
					on = 'keyup paste';
					break;
				case 'search':
					on = 'input paste';
					break;
				case 'date':
				case 'datetime':
				case 'datetime-local':
				case 'month':
				case 'time':
				case 'week':
				case 'file':
				case 'range':
				case 'color':
					on = 'change';
					break;
				default: // number and other future (HTML6+) inputs
					on = 'keyup paste change';
			}
			
			return {
				on: on,
				getValue: function() { return this.value; },
				setValue: function( v ) {
					if( this.value != v ) {
						this.value = v;
					}
				}
			}
		},
		select: function( multiple ) {
			var i;
			if( multiple ) {
				return {
					on: 'change',
					getValue: function() {
						return [].slice.call( this.options )
							.filter( function( o ) { return o.selected; })
							.map( function( o ) { return o.value; });
					},
					setValue: function( v ) {
						v = typeof v == 'string' ? [ v ] : v;
						for( i = this.options.length - 1; i >= 0; i-- ) {
							this.options[ i ].selected = ~v.indexOf( this.options[ i ].value );
						}
					}
				};
			} else {
				return {
					on: 'change',
					getValue: function() { return this.value; },
					setValue: function( v ) {
						var _this = this,
							options;
							
						_this.value = v;
						
						if( !v ) {
							options = _this.options;
							for( i = options.length - 1; i >= 0; i-- ) {
								if( !options[ i ].value ) {
									options[ i ].selected = true;
								}
							}
						}
					}
				};
			}
		},
		visibility: function( value ) {
			value = typeof value == 'undefined' ? true : value;
			return oneWayBinder( function( v ) {
				this.style.display = value ? ( v ? '' : 'none' ) : ( v ? 'none' : '' );
			});
		}
	};
}));