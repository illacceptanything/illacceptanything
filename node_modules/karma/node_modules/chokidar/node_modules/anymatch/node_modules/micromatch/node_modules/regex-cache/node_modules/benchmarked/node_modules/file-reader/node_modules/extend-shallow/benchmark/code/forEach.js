'use strict';

var slice = require('array-slice');

module.exports = function extend(o) {
  var args = slice(arguments, 1);

  args.forEach(function (obj) {
    for (var key in obj) {
      if (obj.hasOwnProperty(key)) {
        o[key] = obj[key];
      }
    }
  });

  return o;
};

