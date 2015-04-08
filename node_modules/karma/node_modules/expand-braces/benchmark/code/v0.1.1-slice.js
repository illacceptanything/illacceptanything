'use strict';

var braces = require('braces');
var slice = require('array-slice');
var uniq = require('array-uniq');

module.exports = function expand(val, fn) {
  var args = slice(arguments);
  args[0] = Array.isArray(args[0]) ? args[0] : [args[0]];
  var len = val.length;
  var arr = [];
  var i = 0;

  while (i < len) {
    arr = arr.concat(braces(val[i++], fn));
  }
  return uniq(arr);
};
