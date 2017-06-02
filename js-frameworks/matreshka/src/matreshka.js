"use strict";
if ( typeof define === 'function' && define.amd ) {
	define( 'matreshka', [
		'matreshka_dir/matreshka-core',
		'matreshka_dir/matreshka-object',
		'matreshka_dir/matreshka-array'
	], function( MK, MK_Object, MK_Array, MK_binders ) {
		return MK;
	});
}