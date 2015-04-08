'use strict';

var braces = require('braces');
var uniq = require('array-uniq');
var slice = require('array-slice');

module.exports = function expand() {
  var args = slice(arguments);
  var fn;

  if (typeof args[args.length - 1] === 'function') {
    fn = args.pop();
  }

  var len = args.length;
  var arr = [];
  var i = 0;

  while (i < len) {
    var current = args[i++];
    if (Array.isArray(current)) {
      var clen = current.length;
      var j = 0;
      while (j < clen) {
        arr.push.apply(arr, braces(current[j++], fn));
      }
    } else {
      arr.push.apply(arr, braces(current, fn));
    }
  }
  return uniq(arr);
};
