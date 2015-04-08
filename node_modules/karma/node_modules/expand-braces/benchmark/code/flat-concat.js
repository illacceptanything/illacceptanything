'use strict';

var braces = require('braces');
var flat = require('arr-flatten');
var uniq = require('array-uniq');
var slice = require('array-slice');

module.exports = function expand() {
  var args = slice(arguments);
  var fn;

  if (typeof args[args.length - 1] === 'function') {
    fn = args.pop();
  }

  args = flat(args);

  var len = args.length;
  var arr = [];
  var i = 0;

  while (i < len) {
    arr = arr.concat(braces(args[i++], fn));
  }
  return uniq(arr);
};
