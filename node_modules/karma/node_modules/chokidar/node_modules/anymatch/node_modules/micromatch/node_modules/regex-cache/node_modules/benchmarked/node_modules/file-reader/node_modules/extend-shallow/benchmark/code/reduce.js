'use strict';

var slice = require('array-slice');

module.exports = function extend(o) {
  var args = slice(arguments, 1);

  return args.reduce(function (acc, value, i) {
    for (var key in value) {
      if (value.hasOwnProperty(key)) {
        acc[key] = value[key];
      }
    }
    return acc;
  }, o);
};
