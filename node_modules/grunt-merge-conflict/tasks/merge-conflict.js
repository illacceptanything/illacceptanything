'use strict';

var checkFile = require('./lib/check-file');

module.exports = function (grunt) {
  grunt.registerMultiTask('merge-conflict', 'Check for merge conflict markers', function () {
    var done = this.async();
    grunt.util.async.forEach(this.filesSrc, function (file, next) {
      if (grunt.file.isFile(file)) {
        var fileContents = grunt.file.read(file);
        var errs;
        if (errs = checkFile(fileContents)) {
          errs.forEach(function (err) {
            grunt.log.errorlns(file + ' has merge conflict marker at line ' + err);
          });
          next(true);
        }
      }
      next();
    }, function (failed) {
      if (failed) {
        grunt.warn('merge-conflict check failed.');
      }
      done();
    });
  });
};
