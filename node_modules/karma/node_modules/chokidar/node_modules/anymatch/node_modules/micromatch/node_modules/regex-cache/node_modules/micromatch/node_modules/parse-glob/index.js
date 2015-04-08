/*!
 * parse-glob <https://github.com/jonschlinkert/parse-glob>
 *
 * Copyright (c) 2015, Jon Schlinkert.
 * Licensed under the MIT License.
 */

'use strict';

var findBase = require('glob-base');
var pathRe = require('glob-path-regex');
var isGlob = require('is-glob');

/**
 * Expose `parseGlob` and cache results in memory
 */

module.exports = function (pattern, getbase) {
  return globCache(parseGlob, pattern, getbase);
};

/**
 * Parse a glob pattern into tokens.
 *
 * When no paths or '**' are in the glob, we use a
 * different strategy for parsing the filename, since
 * file names can contain braces and other difficult
 * patterns. such as:
 *
 *  - `*.{a,b}`
 *  - `(**|*.js)`
 */

function parseGlob(pattern, getbase) {
  var glob = pattern;
  var tok = {path: {}, is: {}, match: {}};
  var path = {};

  // store original pattern
  tok.original = pattern;
  tok.pattern = pattern;
  path.whole = tok.pattern;

  // Boolean values
  tok.is.glob = isGlob(glob);
  tok.is.negated = glob.charAt(0) === '!';
  tok.is.globstar = glob.indexOf('**') !== -1;

  var braces = glob.indexOf('{') !== -1;
  if (tok.is.glob && braces) {
    tok.is.braces = true;
    glob = glob.substr(0, braces) + escape(glob.substr(braces));
  }

  // if there is no `/` and no `**`, this means our
  // pattern can only match file names
  if (glob.indexOf('/') === -1 && !tok.is.globstar) {
    path.dirname = '';
    path.filename = tok.original;
    tok.is.globstar = false;

    var basename = /^([^.]*)/.exec(glob);
    if (basename) {
      path.basename = basename[0] || '';
      path.extname = glob.substr(path.basename.length);
    } else {
      path.basename = tok.original;
      path.extname = '';
    }

    path.ext = path.extname.split('.').slice(-1)[0];
    if (braces) {
      path.basename = unescape(path.basename);
    }

  // we either have a `/` or `**`
  } else {
    var m = pathRe().exec(glob) || [];
    path.dirname = m[1];
    path.filename = glob.substr(path.dirname.length);

    // does the filename have a `.`?
    var dot = path.filename.indexOf('.', 1);
    if (dot !== -1) {
      path.basename = path.filename.substr(0, dot);
      path.extname = path.filename.substr(dot);
      path.ext = path.extname.substr(path.extname.indexOf('.', 1));
    } else if (path.filename.charAt(0) === '.') {
      path.basename = '';
      path.extname = path.filename;
    } else {
      path.basename = path.filename;
      path.extname = '';
    }

    path.ext = path.extname.split('.').slice(-1)[0];
    // remove any escaping that was applied for braces
    if (braces) {
      path = unscapeBraces(path);
    }
  }

  tok.is.dotfile = path.filename.charAt(0) === '.';
  tok = matchesDotdirs(tok, path);
  tok.path = path;

  // get the `base` from glob pattern
  if (getbase) {
    var segs = findBase(tok.pattern);
    tok.pattern = segs.pattern;
    tok.base = segs.base;

    if (tok.is.glob === false) {
      tok.base = tok.path.dirname;
      tok.pattern = tok.path.filename;
    }
  }
  return tok;
}

/**
 * Updates the tokens to reflect if the pattern
 * matches dot-directories
 *
 * @param  {Object} `tok` The tokens object
 * @param  {Object} `path` The path object
 * @return {Object}
 */

function matchesDotdirs(tok, path) {
  tok.is.dotdir = false;
  if (path.dirname.indexOf('/.') !== -1) {
    tok.is.dotdir = true;
  }
  if (path.dirname.charAt(0) === '.' && path.dirname.charAt(1) !== '/') {
    tok.is.dotdir = true;
  }
  return tok;
}

/**
 * Unescape brace patterns in each segment on the
 * `path` object.
 *
 * TODO: this can be reduced by only escaping/unescaping
 * segments that need to be escaped based on whether
 * or not the pattern has a directory in it.
 *
 * @param  {Object} `path`
 * @return {Object}
 */

function unscapeBraces(path) {
  path.dirname = path.dirname ? unescape(path.dirname) : '';
  path.filename = path.filename ? unescape(path.filename) : '';
  path.basename = path.basename ? unescape(path.basename) : '';
  path.extname = path.extname ? unescape(path.extname) : '';
  path.ext = path.ext ? unescape(path.ext) : '';
  return path;
}

/**
 * Cache the glob string to avoid parsing the same
 * pattern more than once.
 *
 * @param  {Function} fn
 * @param  {String} pattern
 * @param  {Options} options
 * @return {RegExp}
 */

function globCache(fn, pattern, getbase) {
  var key = pattern + (getbase || '');
  return cache[key] || (cache[key] = fn(pattern, getbase));
}

/**
 * Expose the glob `cache`
 */

var cache = module.exports.cache = {};

/**
 * Escape/unescape utils
 */

function escape(str) {
  return str.replace(/.*\{([^}]*?)}.*$/g, function (match, inner) {
    if (!inner) { return match; }
    return match.split(inner).join(esc(inner));
  });
}

function esc(str) {
  str = str.split('/').join('__ESC_SLASH__');
  str = str.split('.').join('__ESC_DOT__');
  return str;
}

function unescape(str) {
  str = str.split('__ESC_SLASH__').join('/');
  str = str.split('__ESC_DOT__').join('.');
  return str;
}
