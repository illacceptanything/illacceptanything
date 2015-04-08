/**
 * relative <https://github.com/jonschlinkert/relative>
 * Copyright (c) 2014 Jon Schlinkert, contributors.
 * Licensed under the MIT license.
 */

'use strict';

var fs = require('fs');
var path = require('path');
var normalize = require('normalize-path');


// True if the filepath is a directory.
function isDir() {
  var filepath = path.join.apply(path, arguments);
  if (!fs.existsSync(filepath)) {
    return false;
  }
  return fs.statSync(filepath).isDirectory();
}


/**
 * ## relative
 *
 * Return the relative path from `a` to `b`.
 *
 * **Example**:
 *
 * ```js
 * var relative = require('relative');
 * relative('test/fixtures/foo.txt', 'docs/new/file.txt');
 * //=> '../../docs/new/file.txt'
 * ```
 *
 * @param   {String} `from`
 * @param   {String} `to`
 * @return  {String}
 */

var relative = module.exports = function(a, b) {
  if(arguments.length === 1) {b = a; a = process.cwd();}
  a = !isDir(a) ? path.dirname(a) : a;
  var rel = path.relative(path.resolve(a), path.resolve(b));
  return normalize(rel);
};


/**
 * ## .toBase
 *
 * Get the path relative to the given base path.
 *
 * **Example**:
 *
 * ```js
 * relative.toBase('test/fixtures', 'test/fixtures/docs/new/file.txt');
 * //=> 'docs/new/file.txt'
 * ```
 *
 * @param   {String}  `basepath`  The base directory
 * @param   {String}  `filepath`  The full filepath
 * @return  {String}            The relative path
 */

relative.toBase = function (basepath, filepath) {
  filepath = path.resolve(filepath);
  basepath = path.resolve(basepath);

  if (filepath.indexOf(basepath) === 0) {
    filepath = filepath.replace(basepath, '');
  }
  filepath = normalize(filepath);

  // Remove leading slash.
  return filepath.replace(/^\//, '');
};
