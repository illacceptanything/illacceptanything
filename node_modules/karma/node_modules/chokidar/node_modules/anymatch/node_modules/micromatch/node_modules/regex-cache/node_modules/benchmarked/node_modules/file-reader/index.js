/*!
 * file-reader <https://github.com/jonschlinkert/file-reader>
 *
 * Copyright (c) 2014-2015, Jon Schlinkert.
 * Licensed under the MIT license.
 */

'use strict';

var fs = require('fs');
var path = require('path');
var mapFiles = require('map-files');
var extend = require('extend-shallow');
var yaml = require('read-yaml');

/**
 * Expose `readFiles`
 */

module.exports = readFiles;

function readFiles(patterns, options) {
  return mapFiles(patterns, extend({
    renameKey: camelize,
    read: readFile
  }, options));
}

/**
 * Expose `readFile`
 */

module.exports.file = readFile;

function readFile(fp, options) {
  var ext = path.extname(fp);
  if (!reader.hasOwnProperty(ext)) {
    ext = '.txt';
  }
  return reader[ext](path.resolve(fp), options);
}

/**
 * This is just a minimal start, pull requests welcome
 * for adding extensions/readers to the list.
 *
 * @param {String} `ext`
 * @return {Function} The file reader to use for the given `ext`
 * @api private
 */

var reader = {
  // requireable
  '.js': require,
  '.json': require,

  // common string formats
  '.txt': readString,
  '.md': readString,
  '.markdown': readString,
  '.mdown': readString,

  '.hbs': readString,
  '.htm': readString,
  '.html': readString,
  '.slim': readString,
  '.swig': readString,
  '.tmpl': readString,

  '.css': readString,
  '.less': readString,
  '.sass': readString,
  '.scss': readString,
  '.styl': readString,

  // common object formats
  '.yaml': readYaml,
  '.yml': readYaml,
};

/**
 * Camelcase rename function to pass to [map-files].
 *
 * @param  {String} `fp`
 * @return {String}
 */

function camelize(fp) {
  var str = path.basename(fp, path.extname(fp));
  if (/\./.test(str)) {
    str = str.split('.')[0];
  }
  if (str.length === 1) {
    return str;
  }
  str = str.replace(/^[-_.\s]+/, '').toLowerCase();
  return str.replace(/[-_.]+(\w|$)/g, function (_, ch) {
    return ch.toUpperCase();
  });
}

function readString(fp) {
  return fs.readFileSync(fp, 'utf8');
}

function readYaml(fp, options) {
  return yaml.sync(fp);
}