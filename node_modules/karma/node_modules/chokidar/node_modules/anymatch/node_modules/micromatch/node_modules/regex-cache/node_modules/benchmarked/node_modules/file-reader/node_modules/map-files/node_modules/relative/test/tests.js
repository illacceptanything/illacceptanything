/**
 * Relative <https://github.com/jonschlinkert/relative>
 * Copyright (c) 2014 Jon Schlinkert, contributors.
 * Licensed under the MIT license.
 */

'use strict';

var expect = require('chai').expect;
var path = require('path');
var cwd = process.cwd();

var relative = require('../');

describe('relative', function() {
  it('should resolve the relative path from a file to a file', function() {
    var expected = '../../docs/new/file.txt';
    var actual = relative('test/fixtures/foo.txt', 'docs/new/file.txt');
    expect(actual).to.eql(expected);
  });

  it('should resolve the relative path from a file to a directory', function() {
    var expected = '../../docs';
    var actual = relative('test/fixtures/foo.txt', 'docs');
    expect(actual).to.eql(expected);
  });

  it('should resolve the relative path from a directory to a directory', function() {
    var expected = '../docs';
    var actual = relative('test/fixtures', 'docs');
    expect(actual).to.eql(expected);
  });

  it('should resolve the relative path from a directory to a file', function() {
    var expected = '../docs/foo.txt';
    var actual = relative('test/fixtures', 'docs/foo.txt');
    expect(actual).to.eql(expected);
  });

  it('should resolve the relative path to process.cwd()', function() {
    var expected = '../..';
    var actual = relative('test/fixtures/foo.txt', process.cwd());
    expect(actual).to.eql(expected);
  });

  it('should resolve the relative path from process.cwd()', function() {
    var expected = 'test/fixtures/foo.txt';
    var actual = relative(process.cwd(), 'test/fixtures/foo.txt');
    expect(actual).to.eql(expected);
  });
});


describe('relative.toBase', function() {
  it('should resolve the relative path from a base path to a file', function() {
    var expected = 'docs/new/file.txt';
    var actual = relative.toBase('test/fixtures', 'test/fixtures/docs/new/file.txt');
    expect(actual).to.eql(expected);
  });

  it('should resolve the relative path from a base path to a file', function() {
    var expected = 'docs/new/file.txt';
    var actual = relative.toBase('test/fixtures/', 'test/fixtures/docs/new/file.txt');
    expect(actual).to.eql(expected);
  });

  it('should resolve the relative path from a base path to a file', function() {
    var expected = 'docs/new';
    var actual = relative.toBase('test/fixtures/', 'test/fixtures/docs/new');
    expect(actual).to.eql(expected);
  });

  it('should resolve the relative path from a base path to a file', function() {
    var expected = 'docs/new';
    var actual = relative.toBase('test/fixtures/', 'test/fixtures/docs/new/');
    expect(actual).to.eql(expected);
  });
});
