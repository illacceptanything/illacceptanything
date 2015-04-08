'use strict';

var slice = require('array-slice');

module.exports = function extend(o) {
  var len = arguments.length;

  for (var i = 0; i < len; i++) {
    var obj = arguments[i];

    for (var key in obj) {
      if (!!obj[key]) {
        o[key] = obj[key];
      }
    }
  }

  return o;
};
