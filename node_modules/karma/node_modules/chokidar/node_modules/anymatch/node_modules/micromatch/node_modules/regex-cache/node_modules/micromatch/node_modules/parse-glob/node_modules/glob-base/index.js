/*!
 * glob-base <https://github.com/jonschlinkert/glob-base>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var path = require('path');
var isGlob = require('is-glob');
var parent = require('glob-parent');

module.exports = function globBase(glob) {
  if (typeof glob !== 'string') {
    throw new TypeError('glob-base expects a string.');
  }

  var res = {};
  res.base = parent(glob);

  if (res.base !== '.') {
    res.pattern = glob.substr(res.base.length);
    if (res.pattern.charAt(0) === '/') {
      res.pattern = res.pattern.substr(1);
    }
  } else {
    res.pattern = glob;
  }

  if (res.base === glob) {
    res.base = dirname(glob);
    res.pattern = res.base === '.' ? glob : glob.substr(res.base.length);
  }

  if (res.pattern.substr(0, 2) === './') {
    res.pattern = res.pattern.substr(2);
  }

  if (res.pattern.charAt(0) === '/') {
    res.pattern = res.pattern.substr(1);
  }
  return res;
}

function dirname(glob) {
  if (glob[glob.length - 1] === '/') {
    return glob;
  }
  return path.dirname(glob);
}
