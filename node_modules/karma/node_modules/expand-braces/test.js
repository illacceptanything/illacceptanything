/*!
 * expand-braces <https://github.com/jonschlinkert/expand-braces>
 *
 * Copyright (c) 2014 Jon Schlinkert, contributors.
 * Licensed under the MIT License
 */

'use strict';

var should = require('should');
var expand = require('./');

describe('given an array of patterns with braces', function () {
  describe('braces', function () {
    it('should work with a list of strings', function () {
      expand('x{a,b,c}z', 'x{a,b,c}z').should.eql(['xaz', 'xbz', 'xcz']);
    });

    it('should work with arrays of strings', function () {
      expand(['x{a,b,c}z', 'x{a,b,c}z']).should.eql(['xaz', 'xbz', 'xcz']);
    });

    it('should work with no braces', function () {
      expand(['abc', 'xyz']).should.eql(['abc', 'xyz']);
    });

    it('should uniquify the result.', function () {
      expand(['x{a,b,c}z', 'x{a,b,c}z']).should.eql(['xaz', 'xbz', 'xcz']);
    });

    it('should work with one value', function () {
      expand(['a{b}c', 'a/b/c{d}e']).should.eql(['abc', 'a/b/cde']);
    });

    it('should work with nested non-sets', function () {
      expand(['{a-{b,c,d}}', '{a,{a-{b,c,d}}}']).should.eql(['a-b', 'a-c', 'a-d', 'a']);
    });

    it('should work with commas.', function () {
      expand(['a{b,}c', 'a{,b}c']).should.eql(['abc', 'ac']);
    });

    it('should expand sets', function () {
      expand(['a/{x,y}/c{d}e', 'a/b/c/{x,y}']).should.eql(['a/x/cde', 'a/y/cde', 'a/b/c/x', 'a/b/c/y']);
    });

    it('should throw an error when imbalanced braces are found.', function () {
      (function () {
        expand(['a/{b,c}{d{e,f}g']);
      }).should.throw('imbalanced brace in: a/{b,c}{deg');
    });
  });

  describe('range expansion', function () {
    it('should expand numerical ranges', function () {
      expand(['{1..3}', '{4..8}']).should.eql(['1', '2', '3', '4', '5', '6', '7', '8']);
    });

    it('should honor padding', function () {
      expand(['{00..05}', '{01..03}', '{000..005}']).should.eql(['00', '01', '02', '03', '04', '05', '000', '001', '002', '003', '004', '005']);
    });

    it('should expand alphabetical ranges', function () {
      expand(['{a..e}', '{A..E}']).should.eql(['a', 'b', 'c', 'd', 'e', 'A', 'B', 'C', 'D', 'E']);
    });

    it('should use a custom function for expansions.', function () {
      var i = 0;
      var range = expand(['{a..e}', '{f..h}'], function (str) {
        return str + i++;
      });
      range.should.eql(['a0', 'b1', 'c2', 'd3', 'e4', 'f5', 'g6', 'h7']);
    });

    it('should use a custom function for expansions.', function () {
      var range = expand(['{a..e}', '{f..h}'], function (str) {
        return '_' + str;
      });
      range.should.eql(['_a', '_b', '_c', '_d', '_e', '_f', '_g', '_h']);
    });
  });

  describe('should expand a combination of nested sets and ranges.', function () {
    it('should expand sets', function () {
      expand(['a/{x,{1..5},y}/c{d}e']).should.eql(['a/x/cde', 'a/1/cde', 'a/y/cde', 'a/2/cde', 'a/3/cde', 'a/4/cde', 'a/5/cde']);
    });
  });
});
