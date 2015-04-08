'use strict';

/* global Promise: true */

var Promise = require('bluebird');
var checkFile = require('./lib/check-file');
var defaultDisallowed = require('./lib/default-disallowed');

module.exports = function (grunt) {

  grunt.registerMultiTask('ddescribe-iit', 'Check for instances of ddescribe and iit', function () {
    var done = this.async();
    var options = this.options({
      // Default values
      disallowed: defaultDisallowed
    });

    // map files -> arrays of errors
    // (we want to report all the errors, not just the first one)
    Promise.map(this.filesSrc, function (file) {
      return new Promise(function (resolve) {
        var errs;
        if (grunt.file.isFile(file)) {
          var fileContents = grunt.file.read(file);
          errs = checkFile(fileContents, options.disallowed);
          if (errs) {
            errs.forEach(function (err) {
              grunt.log.error(file + ' has `' + err.str + '` at line ' + err.line);
            });
          }
        }
        resolve(errs);
      });

    }).filter(function (errs) {
      return !!errs && !!errs.length;

    }).then(function (errsArray) {
      var success = !errsArray.length;
      done(success);

    }).catch(function (error) {
      grunt.log.error('Unexpected error');
      grunt.log.error(error);
      done(false);

    });
  });

};
