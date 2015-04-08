'use strict';

var should = require('should');

var checkFile = require('../tasks/lib/check-file');

describe('check-file', function () {

  it('should return undefined if there are no problems', function () {
    should.not.exist(checkFile(""));
  });

  it('should return an array if there is a merge conflict', function () {
    should.exist(checkFile(">>>>>>>"));
    should.exist(checkFile("<<<<<<<"));
    should.exist(checkFile("======="));
  });

  it('should not have false negatives', function () {
    should.not.exist(checkFile("// ======="));
  });

  it('should give the line number of the problem', function () {
    var problems = checkFile("\n\n=======");
    problems.length.should.equal(1);
    should.equal(problems[0], 3);
  });

});
