/*!
 * normalize-path
 * https://github.com/jonschlinkert/normalize-path
 *
 * Copyright (c) 2014 Jon Schlinkert, contributors.
 * Licensed under the MIT License
 */

'use strict';

var path = require('path');

module.exports = function(filepath) {
  return path.normalize(filepath)
    .replace(/\\/g, '/')
    .replace(/\/$/g, '');
};