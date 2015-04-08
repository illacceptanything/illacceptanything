'use strict';

var slice = require('array-slice');

module.exports = function extend(o) {
  var args = slice(arguments, 1);
  var len = args.length;
  var i = 0;

  while (len--) {
    var obj = args[i++];
    if (obj) {
      var keys = Object.keys(obj);
      var len = keys.length;

      for (var j = 0; j < len; j++) {
        var key = keys[j];
        if (obj.hasOwnProperty(key)) {
          o[key] = obj[key];
        }
      }
    }

  }
  return o;
};
