'use strict';

var fs = require('fs');
var path = require('path');
var chalk = require('chalk');
var glob = require('glob');

var fixtures = __dirname + '/fixtures';
var code = __dirname + '/code';

glob.sync('*.js', {cwd: code}).forEach(function (fp) {
  var fn = require(path.resolve(code, fp));
  var name = path.basename(fp, path.extname(fp));

  glob.sync('*.js', {cwd: fixtures}).forEach(function (fixture) {
    fixture = path.resolve(fixtures, fixture);

    var base = path.basename(fixture, path.extname(fixture));
    console.log(chalk.bold(name + ' [' + base + ']') + ':', fn.apply(fn, require(fixture)));
  });
});
