'use strict';

var path = require('path');
var util = require('util');
var ansi = require('ansi');
var chalk = require('chalk');
var read = require('file-reader');
var hasValues = require('has-values');
var forOwn = require('for-own');
var Benchmark = require('benchmark');
var extend = require('extend-shallow');
var cursor = ansi(process.stdout);

/**
 * Suite constructor.
 */

function Suite(options) {
  this._fixtures = {};
  this._fns = {};

  this.options = extend({
    cwd: process.cwd(),
    name: function(fp) {
      var ext = path.extname(fp);
      return path.basename(fp, ext);
    },
  }, options);
}

/**
 * Define fixtures to run benchmarks against.
 *
 * @param  {String|Array} `patterns` Filepath(s) or glob patterns.
 * @param  {Options} `options`
 * @api public
 */

Suite.prototype.fixtures = function(patterns, options) {
  options = extend({}, this.options, options);
  this._fixtures = read(patterns, options);
  return this;
};

/**
 * Specify the functions to be benchmarked.
 *
 * @param  {String|Array} `patterns` Filepath(s) or glob patterns.
 * @param  {Options} `options`
 * @api public
 */

Suite.prototype.add = function(patterns, options) {
  options = extend({}, this.options, options);
  this._fns = read(patterns, options);
  return this;
};

/**
 * Run the benchmarks
 *
 * @param  {Object} `options`
 * @param  {Function} `cb`
 * @param  {Object} `thisArg`
 * @api public
 */

Suite.prototype.run = function(options, cb, thisArg) {
  var i = 0;

  if (typeof options == 'function') {
    thisArg = cb;
    cb = options;
    options = {};
  }

  options = extend({}, this.options, options);
  var fixtures = this._fixtures;
  var add = this._fns;

  if (options.fixtures) {
    fixtures = read(options.fixtures, options);
  }

  if (options.add) {
    add = read(options.add, options);
  }

  forOwn(fixtures, function (args, name) {
    if (typeof cb == 'function') {
      args = cb(args);
    }

    args = Array.isArray(args) ? args : [args];

    var lead = '';
    if (options.showArgs) {
      lead = ' - ' + util.inspect(args, null, 2).replace(/[\s\n]+/g, ' ');
    }

    var benchmark = new Benchmark.Suite(name, {
      name: name,
      onStart: function () {
        console.log(chalk.gray('#%s: %s'), ++i, name, lead);
      },
      onComplete: function () {
        cursor.write('\n');
      }
    });

    forOwn(add, function (fn, fnName) {
      benchmark.add(fnName, {
        onCycle: function onCycle(event) {
          cursor.horizontalAbsolute();
          cursor.eraseLine();
          cursor.write('  ' + event.target);
        },
        onComplete: function () {
          if (options.result) {
            var res = fn.apply(null, args);
            var msg = chalk.bold('%j');
            if (!hasValues(res)) {
              msg = chalk.red('%j');
            }
            console.log(chalk.gray('  result: ') + msg, res);
          } else {
            cursor.write('\n');
          }
        },
        fn: function () {
          fn.apply(thisArg, args);
          return;
        }
      });

      if (options.sample) {
        console.log('> ' + fnName + ':\n  %j', fn.apply(null, options.sample));
      }
    });

    benchmark.on('complete', function () {
      if (Object.keys(add).length > 1) {
        var fastest = chalk.bold(this.filter('fastest').pluck('name'));
        console.log(chalk.gray('  fastest is ') + fastest);
      }
    });

    benchmark.run();
  });

  return this;
};


/**
 * Expose `Suite`
 */

module.exports = Suite;
