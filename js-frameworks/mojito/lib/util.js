/*
 * Copyright (c) 2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint node:true, nomen: true, stupid:true*/

'use strict';

var libfs = require('fs'),
    libpath = require('path'),
    libyaml = require('js-yaml');

/**
Extends object with properties from other objects.

    var a = { foo: 'bar' }
      , b = { bar: 'baz' }
      , c = { baz: 'xyz' };

    utils.extends(a, b, c);
    // a => { foo: 'bar', bar: 'baz', baz: 'xyz' }

@method extend
@param {Object} obj the receiver object to be extended
@param {Object*} supplier objects
@return {Object} The extended object
**/
function extend(obj) {
    Array.prototype.slice.call(arguments, 1).forEach(function (source) {
        var key;

        if (!source) { return; }

        for (key in source) {
            if (source.hasOwnProperty(key)) {
                obj[key] = source[key];
            }
        }
    });

    return obj;
}

/**
Produces a normalized web path by joining all the parts and normalizing the
filesystem-like path into web compatible url. This is useful when you have to
generate urls based on filesystem path where unix uses `/` and windows uses `\\`.
Node is pretty smart and it will do the heavy lifting, we just need to adjust
the separtor so it uses the `/`. This method also support relative and absolute
paths.

    util.webpath('foo/bar' ,'baz');
    // => foo/bar/baz
    util.webpath('foo\\bar', 'baz/');
    // => foo/bar/baz/
    util.webpath('./foo/bar', './baz');
    // => foo/bar/baz
    util.webpath(['foo', 'bar', 'baz']);
    // => foo/bar/baz

@method webpath
@param {Array|String*} url the list of parts to be joined and normalized
@return {String} The joined and normalized url
**/
function webpath(url) {
    var args = [].concat.apply([], arguments),
        parts = libpath.join.apply(libpath, args).split(libpath.sep);
    return parts.join('/');
}

/**
 * Reads and parses a JSON or YAML structured file.
 *
 * NOTE: Does not parse YCB bundles.
 *
 * @method readConfig
 * @param {string} fullPath path to JSON or YAML file
 * @return {user-defined} contents of file as an object
 */
function readConfig(fullPath) {
    var extensions = ['.yml', '.yaml', '.json'],
        basename,   // everything except the extension
        i,
        json = false,
        raw,
        obj;

    basename = fullPath;
    if (libpath.extname(fullPath)) {
        basename = fullPath.slice(0, libpath.extname(fullPath).length * -1);
    }
    for (i = extensions.length - 1; i >= 0; i -= 1) {
        try {
            fullPath = basename + extensions[i];
            raw = libfs.readFileSync(fullPath, 'utf8');
            try {
                if (i === 2) { // json
                    obj = JSON.parse(raw);
                    json = true;
                } else { // yaml or yml
                    obj = libyaml.load(raw);
                    if (json) {
                        console.warn('WARN: ' + basename + extensions[2] + ' exists but ' + extensions[i] + ' file will be used instead');
                    }
                }
                // TODO: what happen when one of them exists?
                //       and what then more than one exists?
            } catch (parseErr) {
                throw new Error(parseErr);
            }
        } catch (err) {
            if (err.errno !== 34) { // if the error was not "no such file or directory" report it
                throw new Error("Error parsing file: " + fullPath + "\n" + err);
            }
        }
    }
    if (!obj) {
        obj = {};
    }
    return obj;
}

module.exports = {
    extend: extend,
    readConfig: readConfig,
    webpath: webpath
};
