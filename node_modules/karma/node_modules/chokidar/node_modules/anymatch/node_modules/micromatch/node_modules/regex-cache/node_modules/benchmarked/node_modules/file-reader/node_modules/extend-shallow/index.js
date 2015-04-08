'use strict';

var slice = require('array-slice');

/**
 * Extend the target `obj` with the properties of other objects.
 *
 * @param  {Object}  `obj` The target object. Pass an empty object to shallow clone.
 * @param  {Objects}
 * @return {Object}
 */

module.exports = function extend(o) {
  if (o == null) {
    return {};
  }

  var args = slice(arguments, 1);
  var len = args.length;

  for (var i = 0; i < len; i++) {
    var obj = args[i];

    for (var key in obj) {
      if (obj.hasOwnProperty(key)) {
        o[key] = obj[key];
      }
    }
  }

  return o;
};
