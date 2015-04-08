'use strict';

var braces = require('braces');

module.exports = function expand(val) {
  val = Array.isArray(val) ? val : [val];

  return val.reduce(function (acc, str) {
    if (acc && acc.indexOf(str) === -1) {
      return acc.concat(braces(str));
    }
  }, []);
};
