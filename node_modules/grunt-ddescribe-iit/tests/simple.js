'use strict';

var should = require('should');

var checkFile = require('../tasks/lib/check-file');
var defaultDisallowed = require('../tasks/lib/default-disallowed');

describe('check-file', function () {

  it('should return undefined if there are no problems', function () {
    should.not.exist(checkFile("it('is a-ok')", defaultDisallowed));
  });

  it('should return an array if there is `iit`', function () {
    should.exist(checkFile("iit('is not a-ok')", defaultDisallowed));
  });

  it('should return an array if there is `ddescribe`', function () {
    should.exist(checkFile("ddescribe('is not a-ok')", defaultDisallowed));
  });

  it('should return an array if there is `fit`', function () {
    should.exist(checkFile("fit('is not a-ok')", defaultDisallowed));
  });

  it('should return an array if there is `fdescribe`', function () {
    should.exist(checkFile("fdescribe('is not a-ok')", defaultDisallowed));
  });

  it('should return an array if there is `it.only`', function () {
    should.exist(checkFile("it.only('is not a-ok')", defaultDisallowed));
  });

  it('should return an array if there is `describe.only`', function () {
    should.exist(checkFile("describe.only('is not a-ok')", defaultDisallowed));
  });

  it('should return an array if there is `xit`', function () {
    should.exist(checkFile("xit('is not a-ok')", defaultDisallowed));
  });

  it('should return an array if there is `xit` after `exit`', function () {
    should.exist(checkFile("exit(0);xit('is not a-ok')", defaultDisallowed));
    should.exist(checkFile("it('is a-ok');exit(0);xit('is not a-ok')", defaultDisallowed));
  });

  it('should report multiple errors', function () {
    should(checkFile("xit('is not a-ok');fit('is not a-ok')", defaultDisallowed)).have.lengthOf(2);
  });

  it('should return an array if there is `xdescribe`', function () {
    should.exist(checkFile("xdescribe('is not a-ok')", defaultDisallowed));
  });

  it('should give the line number of the problem', function () {
    var problems = checkFile("ddescribe('is not a-ok')", defaultDisallowed);
    problems.length.should.equal(1);
    should.equal(problems[0].line, 1);

    problems = checkFile("\n\niit('is not a-ok')", defaultDisallowed);
    problems.length.should.equal(1);
    should.equal(problems[0].line, 3);

    problems = checkFile("\n\n\n\n\nxit('is not a-ok')", defaultDisallowed);
    problems.length.should.equal(1);
    should.equal(problems[0].line, 6);
  });

  it('should not treat `exit` as `xit`', function () {
    should.not.exist(checkFile("exit()", defaultDisallowed));
  });

  it('can use cutom disallowed keywords', function () {
	  should.not.exist(checkFile("xit('is a-ok to use custom disallowed keywords')", ["iit", "ddescribe"]));
  });

  it('passes if there are no disallowed keywords', function () {
	  should.not.exist(checkFile("xit('is a-ok to use custom disallowed keywords')"));
	  should.not.exist(checkFile("xit('is a-ok to use custom disallowed keywords')", []));
	  should.not.exist(checkFile("xit('is a-ok to use custom disallowed keywords')", {}));
  });

});
