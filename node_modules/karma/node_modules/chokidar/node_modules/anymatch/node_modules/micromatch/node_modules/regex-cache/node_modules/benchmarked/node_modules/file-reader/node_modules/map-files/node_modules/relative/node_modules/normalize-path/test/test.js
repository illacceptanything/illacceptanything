/*!
 * normalize-path <https://github.com/jonschlinkert/normalize-path>
 *
 * Copyright (c) 2014 Jon Schlinkert, contributors.
 * Licensed under the MIT License
 */

var expect = require('chai').expect;
var normalize = require('../');


describe('normalize:', function () {
  it('should normalize file paths.', function () {
    expect(normalize('E://foo//bar//baz')).to.equal('E:/foo/bar/baz');
    expect(normalize('E://foo//bar//baz//')).to.equal('E:/foo/bar/baz');
    expect(normalize('E:/foo/bar/baz/')).to.equal('E:/foo/bar/baz');
    expect(normalize('E://foo\\bar\\baz')).to.equal('E:/foo/bar/baz');
    expect(normalize('foo\\bar\\baz')).to.equal('foo/bar/baz');
    expect(normalize('foo\\bar\\baz\\')).to.equal('foo/bar/baz');
    expect(normalize('E://foo/bar\\baz')).to.equal('E:/foo/bar/baz');
    expect(normalize('E:\\\\foo/bar\\baz')).to.equal('E:/foo/bar/baz');
    expect(normalize('//foo/bar\\baz')).to.equal('//foo/bar/baz');
    expect(normalize('//foo\\bar\\baz')).to.equal('//foo/bar/baz');
    expect(normalize('C:\\user\\docs\\Letter.txt')).to.equal('C:/user/docs/Letter.txt');
    expect(normalize('/user/docs/Letter.txt')).to.equal('/user/docs/Letter.txt');
    expect(normalize('C:Letter.txt')).to.equal('C:Letter.txt');
    expect(normalize('\\Server01\\user\\docs\\Letter.txt')).to.equal('/Server01/user/docs/Letter.txt');
    expect(normalize('\\?\\UNC\\Server01\\user\\docs\\Letter.txt')).to.equal('/?/UNC/Server01/user/docs/Letter.txt');
    expect(normalize('\\?\\C:\\user\\docs\\Letter.txt')).to.equal('/?/C:/user/docs/Letter.txt');
    expect(normalize('C:\\user\\docs\\somefile.ext:alternate_stream_name')).to.equal('C:/user/docs/somefile.ext:alternate_stream_name');
    expect(normalize('./cwd')).to.equal('cwd');
    expect(normalize('../../grandparent')).to.equal('../../grandparent');
  });
});