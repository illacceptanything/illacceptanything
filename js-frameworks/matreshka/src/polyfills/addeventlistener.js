(function (root, factory) {
    if (typeof define === 'function' && define.amd) {
        define(factory);
    } else {
        factory();
    }
}(this, function () {
	( function( win, doc, s_add, s_rem ) {
	if( doc[s_add] ) return;
		Element.prototype[ s_add ] = win[ s_add ] = doc[ s_add ] = function( on, fn, self ) {
			return (self = this).attachEvent( 'on' + on, function(e){
				var e = e || win.event;
				e.target = e.target || e.srcElement;
				e.preventDefault  = e.preventDefault  || function(){e.returnValue = false};
				e.stopPropagation = e.stopPropagation || function(){e.cancelBubble = true};
				e.which = e.button ? ( e.button === 2 ? 3 : e.button === 4 ? 2 : e.button ) : e.keyCode;
				fn.call(self, e);
			});
		};
		Element.prototype[ s_rem ] = win[ s_rem ] = doc[ s_rem ] = function( on, fn ) {
			return this.detachEvent( 'on' + on, fn );
		};
	})( window, document, 'addEventListener', 'removeEventListener' );
}));