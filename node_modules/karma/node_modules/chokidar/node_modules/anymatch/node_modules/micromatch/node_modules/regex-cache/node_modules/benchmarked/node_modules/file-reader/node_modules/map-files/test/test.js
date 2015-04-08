/*!
 * map-files <https://github.com/jonschlinkert/map-files>
 *
 * Copyright (c) 2014 Jon Schlinkert, contributors.
 * Licensed under the MIT License
 */

'use strict';

var fs = require('fs');
var path = require('path');
var should = require('should');
var assert = require('assert');
var matter = require('gray-matter');
var glob = require('globby');
var files = require('..');


describe('files', function () {
  it('should load files from a glob pattern.', function () {
    var cache = files('test/fixtures/*.txt');

    cache.should.have.property('a');
    cache.should.have.property('b');
    cache.should.have.property('c');
    cache.should.eql({
      a: { content: 'AAA', path: 'test/fixtures/a.txt' },
      b: { content: 'BBB', path: 'test/fixtures/b.txt' },
      c: { content: 'CCC', path: 'test/fixtures/c.txt' }
    });
  });

  it('should use a cwd.', function () {
    var cache = files('*.txt', {cwd: 'test/fixtures'});

    cache.should.have.property('a');
    cache.should.have.property('b');
    cache.should.have.property('c');
    cache.should.eql({
      a: { content: 'AAA', path: 'test/fixtures/a.txt' },
      b: { content: 'BBB', path: 'test/fixtures/b.txt' },
      c: { content: 'CCC', path: 'test/fixtures/c.txt' }
    });
  });

  it('should rename the key with a custom function.', function () {
    var cache = files('test/fixtures/*.txt', {
      name: function(filepath) {
        return filepath;
      }
    })
    cache.should.have.property('test/fixtures/a.txt');
    cache.should.have.property('test/fixtures/b.txt');
    cache.should.have.property('test/fixtures/c.txt');
  });

  it('should read files with a custom function.', function () {
    var cache = files('test/fixtures/*.txt', {
      read: function(filepath) {
        return matter.read(filepath);
      }
    });
    cache.should.have.property('a', { data: {}, content: 'AAA', orig: 'AAA', path: 'test/fixtures/a.txt' });
    cache.should.have.property('b', { data: {}, content: 'BBB', orig: 'BBB', path: 'test/fixtures/b.txt' });
    cache.should.have.property('c', { data: {}, content: 'CCC', orig: 'CCC', path: 'test/fixtures/c.txt' });
  });

  it('should require files with a custom function.', function () {
    var cache = files('test/fixtures/*.js', {
      read: function (filepath) {
        return {
          path: filepath,
          helper: require(path.resolve(filepath))
        }
      }
    });
    cache.should.have.property('a');
    cache.should.have.property('b');
    cache.should.have.property('c');
    cache['a'].path.should.equal('test/fixtures/a.js');
    cache['a'].helper.should.be.an.object;
    cache['a'].helper.should.be.a.function;
  });

  it('should use multiple custom functions.', function () {
    var cache = files('test/fixtures/*.txt', {
      read: function(filepath) {
        return matter.read(filepath);
      },
      name: function(filepath) {
        return filepath;
      }
    });
    cache.should.have.property('test/fixtures/a.txt', { data: {}, content: 'AAA', orig: 'AAA', path: 'test/fixtures/a.txt' });
    cache.should.have.property('test/fixtures/b.txt', { data: {}, content: 'BBB', orig: 'BBB', path: 'test/fixtures/b.txt' });
    cache.should.have.property('test/fixtures/c.txt', { data: {}, content: 'CCC', orig: 'CCC', path: 'test/fixtures/c.txt' });
  });

  it('readme example #2.', function () {
    var cache = files('test/fixtures/*.js', {
      read: function (fp) {
        return require(path.resolve(fp));
      }
    });
    // console.log(cache);
  });
});