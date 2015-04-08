var Q = require('q');
var glob = require('glob');
var fs = require('fs');
var path = require('path');
var spawn = require('child_process').spawn;

var util = require('./util');

module.exports = function(gulp, plugins, config) {
  return function() {
    return util.forEachSubDirSequential(
      config.dest,
      function(dir) {
        var testDir = path.join(dir, 'test');
        var relativeMasterTestFile = 'test/_all_tests.dart';
        var testFiles = [].slice.call(glob.sync('**/*.server.spec.dart', {
          cwd: testDir
        }));
        if (testFiles.length == 0) {
          // No test files found
          return Q.resolve();
        }
        var header = ['library _all_tests;', ''];
        var main = ['main() {'];
        testFiles.forEach(function(fileName, index) {
          header.push('import "' + fileName + '" as test_' + index + ';');
          main.push('  test_' + index + '.main();');
        });
        header.push('');
        main.push('}');

        var absMasterTestFile = path.join(dir, relativeMasterTestFile);
        fs.writeFileSync(absMasterTestFile, header.concat(main).join('\n'));

        var defer = Q.defer();
        var done = defer.makeNodeResolver();
        util.processToPromise(spawn('dart', ['-c', relativeMasterTestFile], {
          stdio: 'inherit',
          cwd: dir
        })).then(
          function() { done(); },
          function(error) { done(error); }
        );
        return defer.promise;
      }
    );
  };
};
