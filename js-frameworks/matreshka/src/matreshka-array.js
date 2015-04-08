"use strict";
(function (root, factory) {
    if (typeof define == 'function' && define.amd) {
        define( [ 'matreshka_dir/matreshka-core' ], factory );
    } else {
        factory( root.MK );
    }
}(this, function ( MK ) {	
	if( !MK ) {
		throw new Error( 'Matreshka is missing' );
	}
	
	var Array_prototype = Array.prototype,
		slice = Array_prototype.slice,
		// Array methods flags
		/*SIMPLE = 1,
		RETURNS_NEW_ARRAY = 2,
		RETURNS_NEW_TYPE = 3,
		MODIFIES = 4,
		MODIFIES_AND_RETURNS_NEW_TYPE = 5,
		SPLICE = 6,*/
		silentFlag = { silent: true, dontRender: true, skipMediator: true },
		compare = function( a1, a2, i, l ) {
			if ( a1.length != a2.length )
				return false;

			for( i = 0, l = a1.length; i < l; i++ ) {
				if ( a1[i] && a1[i].isMK ? !a1[i].eq(a2[i]) : a1[i] !== a2[i] ) { 
					return false;   
				}           
			}
			
			return true;
		},
		indexOf = MK.isXDR ? function( sought ) {
			var _this = this,
				i, item,
				isMK = sought && sought.isMK;
				
			for( i = 0; i < _this.length; i++ ) {
				item = _this[i];
				if( isMK ? sought.eq( item ) : sought === item ) {
					return i;
				}
			}
			
			return -1;
		} : Array_prototype.indexOf,
		lastIndexOf = MK.isXDR ? function ( sought ) {
			var _this = this,
				i, item,
				isMK = sought && sought.isMK;
				
			for( i = _this.length - 1; i >= 0; i-- ) {
				item = _this[i];
				if( isMK ? sought.eq( item ) : sought === item ) {
					return i;
				}
			}
			
			return -1;
		} : Array_prototype.lastIndexOf,
	
	
	triggerAddone = function( _this, added, i ) {
		if( _this.__events.addone ) {
			for( i = 0; i < added.length; i++ ) {
				_this._trigger( 'addone', {
					self: _this,
					added: added[ i ]
				});
			}
		}
	},
	
	triggerRemoveone = function( _this, removed, i ) {
		if( _this.__events.removeone ) {
			for( i = 0; i < removed.length; i++ ) {
				_this._trigger( 'removeone', {
					self: _this,
					removed: removed[ i ]
				});
			}
		}
	},
	
	createMethod = function( name, hasOptions ) {
		var i;
		
		switch( name ) {
			case 'forEach':
				return function() {
					var _this = this;
					Array_prototype[ name ].apply( MK.isXDR ? _this.toArray() : _this, arguments );
					return _this;
				};
			case 'map':
			case 'filter':
			case 'slice':
				return function() {
					var _this = this;
					return new MK.Array().recreate( Array_prototype[ name ].apply( MK.isXDR ? _this.toArray() : _this, arguments ), silentFlag );
				};
			case 'every':
			case 'some':
			case 'reduce':
			case 'reduceRight':
			case 'toString':
			case 'join':
				return function() {
					var _this = this;
					return Array_prototype[ name ].apply( MK.isXDR ? _this.toArray() : _this, arguments );
				};
			case 'sort':
			case 'reverse':
				return function() {
					var _this = this,
						_arguments = arguments,
						args = slice.call( _arguments, 0, hasOptions ? -1 : _arguments.length ),
						evt = hasOptions ? _arguments[ _arguments.length - 1 ] || {} : {},
						array = _this.toArray(),
						returns = Array_prototype[ name ].apply( array, args );
					
					if( !compare( _this, array ) ) {
						_this.recreate( array, silentFlag );
						
						evt = MK.extend({
							returns: returns,
							args: args,
							originalArgs: slice.call( _arguments ),
							method: name,
							self: _this,
							added: [],
							removed: []
						}, evt );
						
						if( !evt.silent ) {
							_this
								._trigger( name, evt )
								._trigger( 'modify', evt )
							;
						}
						
						if( !evt.dontRender ) {
							_this.processRendering( evt );
						}
					}
					
					return _this;
				};
			case 'push':
			case 'pop':
			case 'unshift':
			case 'shift':
				return function() {
					var _this = this,
						_arguments = arguments,
						args = slice.call( _arguments, 0, hasOptions ? -1 : _arguments.length ),
						evt = hasOptions ? _arguments[ _arguments.length - 1 ] || {} : {},
						array = _this.toArray(),
						returns,
						added,
						removed;
					
					if( !evt.skipMediator && typeof _this._itemMediator == 'function' && ( name == 'unshift' || name == 'push' ) ) {
						for( i = 0; i < args.length; i++ ) {
							args[ i ] = _this._itemMediator.call( _this, args[ i ], i );
						}
					}
					
					returns = Array_prototype[ name ].apply( array, args );
					
					if( !compare( _this, array ) ) {
						_this.recreate( array, silentFlag );
						
						evt = MK.extend({
							returns: returns,
							args: args,
							originalArgs: slice.call( _arguments ),
							method: name,
							self: _this,
							added: added = name == 'push' || name == 'unshift' ? args : [],
							removed: removed = name == 'pop' || name == 'shift' ? [ returns ] : []
						}, evt );
						
						if( !evt.silent ) {
							_this._trigger( name, evt );
							
							if( added.length ) {
								_this._trigger( 'add', evt );
								triggerAddone( _this, added );
							} 
							
							if( removed.length ) { // pop, shift
								_this._trigger( 'remove', evt );
								triggerRemoveone( _this, removed );
							}
							
							_this._trigger( 'modify', evt );
						}
						
						if( !evt.dontRender ) {
							_this.processRendering( evt );
						}
					}
					
					return returns;
				};
			case 'splice': 
				return function() {
					var _this = this,
						_arguments = arguments,
						args = slice.call( _arguments, 0, hasOptions ? -1 : _arguments.length ),
						evt = hasOptions ? _arguments[ _arguments.length - 1 ] || {} : {},
						array = _this.toArray(),
						returns,
						added,
						removed;
					
					if( !evt.skipMediator && typeof _this._itemMediator == 'function' ) {
						for( i = 2; i < args.length; i++ ) {
							args[ i ] = _this._itemMediator.call( _this, args[ i ], i );
						}
					}
					
					returns = Array_prototype[ name ].apply( array, args );
					
					if( !compare( _this, array ) ) {
						_this.recreate( array, silentFlag );
						
						evt = MK.extend({
							returns: returns,
							args: args,
							originalArgs: slice.call( _arguments ),
							method: name,
							self: _this,
							added: added = slice.call( args, 2 ),
							removed: removed = returns
						}, evt );
						
						if( !evt.silent ) {
							_this._trigger( name, evt );
							
							if( added.length ) {
								_this._trigger( 'add', evt );
								triggerAddone( _this, added );
							}
							
							if( removed.length ) {
								_this._trigger( 'remove', evt );
								triggerRemoveone( _this, removed );
							}
							
							_this._trigger( 'modify', evt );
						}
						
						if( !evt.dontRender ) {
							_this.processRendering( evt );
						}
					}
					
					return new MK.Array().recreate( returns, silentFlag );
				};
		}
	},
	
	prototype = {
		'extends': MK,
		isMKArray: true,
		length: 0,
		itemRenderer: null,
		renderIfPossible: true,
		useBindingsParser: false,
		Model: null,
		constructor: function( length ) {
			var _this = this._initMK(),
				al = arguments.length,
				i;
			if( al == 1 && typeof length == 'number' ) {
				_this.length = length;
			} else {
				for( i = 0; i < al; i++ ) {
					_this[ i ] = arguments[ i ];
				}
				_this.length = arguments.length;
			}
		},
		
		mediateItem: function( itemMediator ) {
			var _this = this, i;
			_this._itemMediator = itemMediator;
			for( i = 0; i < _this.length; i++ ) {
				_this[ i ] = itemMediator.call( _this, _this[ i ], i );
			}
			return _this;
		},
		
		_on: function( name, callback, context, xtra ) {
			var _this = this._initMK(),
				f;
				
			if( name.indexOf( '@' ) == 0 ) {
				name = name.slice( 1 );
				f = function( evt ) {
					( evt && evt.added ? evt.added : _this ).forEach( function( item ) {
						item && item.isMK && item._on( name, callback, context || _this );
					});
				};
				
				f._callback = callback;
				_this._on( 'add', f, _this, name );
				f.call( context || _this );
			} else {
				MK.prototype._on.call( _this, name, callback, context, xtra );
			}
			
			return _this;
		},
		
		_off: function( name, callback, context ) {
			var _this = this._initMK(),
				events,
				i;
				
			if( name.indexOf( '@' ) == 0 ) {
				name = name.slice( 1 );
				if( callback ) {
					_this.off( 'add', callback, context );
				} else {
					events = _this.__events.add || [];
					for( i = 0; i < events.length; i++ ) {
						if( events[ i ].xtra == name ) {
							_this.off( 'add', events[ i ].callback );
						}
					}
				}
				
				_this.forEach( function( item ) {
					item.isMK && item.off( name, callback, context );
				}, _this );
			} else {
				MK.prototype._off.call( _this, name, callback, context );
			}
			
			return _this;
		},
		
		recreate: function( array, evt ) {
			array = array || [];
			var _this = this,
				diff = _this.length - array.length,
				was = _this.toArray(),
				prepared,
				i,
				added, removed, now;
				
			evt = evt || {};
			
			if( _this._itemMediator && !evt.skipMediator ) {
				prepared = [];
				for( i = 0; i < array.length; i++ ) {
					prepared[ i ] = _this._itemMediator.call( _this, array[ i ], i );
				}
				array = prepared;
			}
			
			for( i = 0; i < array.length; i++ ) {
				_this[ i ] = array[ i ];
			}
			
			for( i = 0; i < diff; i++ ) {
				_this.remove( i + array.length, { silent: true });
			}
			
			_this.length = array.length;
			
			if( evt.silent && evt.dontRender ) {
				return _this;
			}
			
			now = _this.toArray();
			
			removed = was.length ? was.filter( function( item ) {
				return !~indexOf.call( now, item );
			}) : [];
			
			added = now.length ? now.filter( function( item ) {
				return !~indexOf.call( was, item );
			}) : [];
			
			evt = MK.extend({
				added: added,
				removed: removed,
				was: was,
				now: now,
				method: 'recreate',
				self: _this
			}, evt );
			
			if( !evt.silent ) {
				if( added.length ) {
					_this._trigger( 'add', evt );
					triggerAddone( _this, added );
				}
				
				if( removed.length ) {
					_this._trigger( 'remove', evt );
					triggerRemoveone( _this, removed );
				}
				
				if( added.length || removed.length ) {
					_this
						._trigger( 'recreate', evt )
						._trigger( 'modify', evt )
					;
				}
			}
			
			if( !evt.dontRender ) {
				_this.processRendering( evt );
			}
			
			return _this;
		},
		
		
		toArray: function() {
			var _this = this,
				array,
				i;
			try {
				return slice.call( _this );
			} catch( e ) {
				array = [];
				for( i = 0; i < _this.length; i++ ) {
					array[ i ] = _this[ i ];
				}
				return array;
			}
		},
		
		
		toNative: function() {
			return this.toArray();
		},
		
		/**
		 * @method Matreshka.Array#_initMK
		 * @private
		 */
		_initMK: function() {
			var _this = this;
			
			if( _this.isMKInitialized ) return _this;
				
			return MK.prototype._initMK.call( _this )
				.on( 'change:Model', function() {
					var Model = _this.Model;
					if( Model ) {
						_this.mediateItem( function( item ) {
							return !item || !item.isMK || !item.instanceOf( Model ) ? new Model( item && item.toJSON ? item.toJSON() : item, _this ) : item;
						});
					}
				}, true )
			;
		},
		
		/**
		 * @private
		 * @since 0.1
		 */
		_renderOne: function( item, evt ) {
			var _this = this,
				__id = _this.__id,
				renderer = item.renderer || _this.itemRenderer,
				rendererContext = renderer === item.renderer ? item: _this,
				node = item.bound( __id ),
				$node,
				template;

			if( !item[ __id ] ) {
				item[ __id ] = _this;
			}
			
			if( evt.moveSandbox ) {
				if( node = item.bound( 'sandbox' ) ) {
					item.bindNode( __id, node );
				}
			}
			
			if( !node ) {
				if( typeof renderer == 'function' ) {
					renderer = renderer.call( rendererContext, item );
				}
				
				if( typeof renderer == 'string' && !~renderer.indexOf( '<' ) && !~renderer.indexOf( '{{' ) ) {
					template = rendererContext._getNodes( renderer );
					if( template = template && template[0] ) {
						template = template.innerHTML;
					} else {
						throw Error( 'renderer node is missing: ' + renderer );
					}
				} else {
					template = renderer;
				}
				
				$node = _this.useBindingsParser
					? item._parseBindings( template ) 
					: ( typeof template == 'string' ? MK.$.parseHTML( template.replace( /^\s+|\s+$/g, '' ) ) : MK.$( template ) );
				
				if( item.bindRenderedAsSandbox !== false && $node.length ) {
					item.bindNode( 'sandbox', $node );
				}
				
				item.bindNode( __id, $node );
		
				item._trigger( 'render', {
					node: $node[ 0 ],
					$nodes: $node,
					self: item,
					parentArray: _this
				});
				
				node = $node[0];
			} 
			
			return node;
		},
		
		processRendering: function( evt ) {
			var _this = this,
				__id = _this.__id,
				container = container = _this.bound( 'container' ) || _this.bound(),
				destroyOne = function( item ) {
					if( item && item.isMK ) {
						var node = item.bound( __id );
						item.remove( __id, { silent: true });
						return node;
					}
				},
				renderOne = function( item ) {
					return item
						&& item.isMK
						&& _this.renderIfPossible 
						&& container 
						&& !evt.dontRender 
						&& ( _this.itemRenderer || item && item.renderer )
						&& _this._renderOne( item, evt );
				},
				node,
				i;
			
			switch ( evt.method ) {
				case 'push':
					for( i = _this.length - evt.added.length; i < _this.length; i++ ) {
						if( node = renderOne( _this[ i ] ) ) {
							container.appendChild( node );
						}
					}
					break;
				case 'unshift':
					for( i = evt.added.length - 1; i + 1; i-- ) {
						if( node = renderOne( _this[ i ] ) ) {
							if( container.children ) {
								container.insertBefore( node, container.firstChild );
							} else {
								container.appendChild( node );
							}
						}
					}
					break;
				case 'pull':
				case 'pop':
				case 'shift':
					for( i = 0; i < evt.removed.length; i++ ) {
						if( node = destroyOne( evt.removed[ i ] ) ) {
							container.removeChild( node );
						}
					}
					break;
				case 'sort':
				case 'reverse':
					for( i = 0; i < _this.length; i++ ) {
						if( node = _this[ i ].bound( __id ) ) {
							container.appendChild( node );
						}
					}
					break;
				case 'rerender':
					for( i = 0; i < _this.length; i++ ) {
						if( node = renderOne( _this[ i ] ) ) {
							container.appendChild( node );
						}
					}
					break;
				case 'recreate':
				case 'splice':
					for( i = 0; i < evt.removed.length; i++ ) {
						if( node = destroyOne( evt.removed[ i ] ) ) {
							container.removeChild( node );
						}
					}
					
					for( i = 0; i < _this.length; i++ ) {
						if( node = renderOne( _this[ i ] ) ) {
							container.appendChild( node );
						}
					}
					break;
			}
			
			return _this;
		},
		
		
		rerender: function() {
			return this.processRendering({
				method: 'rerender'
			});
		},
		
		
		hasOwnProperty: function( p ) {
			return p == 'length' || p < this.length && p >= 0;
		},
		
		
		toJSON: function() {
			var _this = this,
				JSON = [],
				i;
				
			for( i = 0; i < _this.length; i++ ) {
				_this[ i ] && _this[ i ].toJSON ? JSON.push( _this[ i ].toJSON() ) : JSON.push( _this[ i ] );
			}
			
			return JSON;
		},
		
		
		concat: function() {
			var args = arguments,
				result = this.toArray(),
				arg,
				i,
				j;
			for( i = 0; i < args.length; i++ ) {
				arg = args[ i ];
				if( arg instanceof Array || arg && arg.instanceOf && arg.instanceOf( MK.Array ) ) {
					for( j = 0; j < arg.length; j++ ) {
						result.push( arg[ i ] );
					}
				}
			}
			
			return new MK.Array().recreate( result );
		},
		
		
		
		pull: function( index, evt ) {
			var _this = this,
				array = _this.toArray(),
				_index = index,
				type = typeof index,
				returns,
				removed;
			
			if( type != 'number' && type != 'string' ) {
				index = _this.indexOf( index );
				if( !~index ) {
					return null;
				}
			}
			
			returns = array.splice( index, 1 )[ 0 ] || null;
			
			if( !compare( array, _this ) ) {
				evt = evt || {};
				
				_this.recreate( array, silentFlag );
				
				evt = MK.extend({
					returns: returns,
					args: [ _index ],
					method: 'pull',
					self: _this,
					added: [],
					removed: removed = returns ? [ returns ] : []
				}, evt );
				
				if( !evt.silent ) {
					_this._trigger( 'pull', evt );
					_this._trigger( 'remove', evt );
					triggerRemoveone( _this, removed );
					_this._trigger( 'modify', evt );
				}
				
				_this.processRendering( evt );
			}
		
			return returns;
		},
		
		// es5-shim doesn't help with indexOf and lastIndexOf*/
		indexOf: indexOf,
		lastIndexOf: lastIndexOf
	};
	
	'push pop unshift shift sort reverse splice map filter slice every some reduce reduceRight forEach toString join'.split( ' ' ).forEach( function( name ) {
		prototype[ name ] = createMethod( name );
	});
	
	'push pop unshift shift sort reverse splice'.split( ' ' ).forEach( function( name ) {
		prototype[ name + '_' ] = createMethod( name, 1 );
	});
	
	prototype.each = prototype.forEach;

	prototype[ typeof Symbol != 'undefined' ? Symbol.iterator : '@@iterator' ] = function() {
		var _this = this,
			i = 0;
		return {
			next: function() {
				if ( i > _this.length - 1 ) {
					return { done: true };
				} else {
					return { done: false, value: _this[ i++ ] };
				}
			}
		};
	};
	
	return MK.Array = MK.Class( prototype );
}));
