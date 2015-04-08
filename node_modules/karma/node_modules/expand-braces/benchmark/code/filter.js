'use strict';

var braces = require('braces');
var uniq = require('array-uniq');
var slice = require('array-slice');

module.exports = function expand() {
  var args = slice(arguments);
  var len = args.length;
  var i = 0;
  var fn;

  var patterns = [];

  while (i < len) {
    var arg = args[i++];

    if (typeof arg === 'string') {
      patterns.push(arg);
    }

    if (Array.isArray(arg)) {
      patterns.push.apply(patterns, arg);
    }

    if (typeof arg === 'function') {
      fn = arg;
      break;
    }
  }

  var plen = patterns.length;
  var arr = [];
  var j = 0;

  while (j < plen) {
    arr.push.apply(arr, braces(patterns[j++], fn));
  }
  return uniq(arr);
};