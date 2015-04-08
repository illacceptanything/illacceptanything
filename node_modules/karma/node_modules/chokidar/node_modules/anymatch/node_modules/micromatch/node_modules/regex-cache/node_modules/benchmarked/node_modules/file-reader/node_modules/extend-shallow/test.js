/*!
 * extend-shallow <https://github.com/jonschlinkert/extend-shallow>
 *
 * Copyright (c) 2014 Jon Schlinkert, contributors.
 * Licensed under the MIT License
 */

'use strict';

var should = require('should');
var extend = require('./');

describe('extend', function () {
  it('should extend the first object with the properties of the other objects.', function () {
    extend({a: 'b'}, {c: 'd'}).should.eql({a: 'b', c: 'd'});
    extend({a: 'b', c: 'd'}, {c: 'e'}).should.eql({a: 'b', c: 'e'});
  });

  it('should return an empty object when args are undefined.', function () {
    extend(null).should.eql({});
    extend(undefined).should.eql({});
  });

  describe('.extend():', function () {
    it('should extend object a with object b:', function () {
      extend({a: {b: 'b'}}, {b: {c: 'c'}}).should.eql({a: {b: 'b'}, b: {c: 'c'}});
    });

    it('should return an empty object when args are undefined:', function () {
      extend().should.eql({});
    });
  });
});