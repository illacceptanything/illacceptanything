/*
 * grunt-contrib-connect
 * http://gruntjs.com/
 *
 * Copyright (c) 2014 "Cowboy" Ben Alman, contributors
 * Licensed under the MIT license.
 */

'use strict';

module.exports = function(grunt) {
  var path = require('path');
  var connect = require('connect');
  var http = require('http');
  var https = require('https');
  var injectLiveReload = require('connect-livereload');
  var open = require('open');
  var portscanner = require('portscanner');
  var async = require('async');

  var MAX_PORTS = 30; // Maximum available ports to check after the specified port

  var createDefaultMiddleware = function createDefaultMiddleware(connect, options) {
    var middlewares = [];
    if (!Array.isArray(options.base)) {
      options.base = [options.base];
    }
    var directory = options.directory || options.base[options.base.length - 1];
    options.base.forEach(function(base) {
      // Serve static files.
      middlewares.push(connect.static(base));
    });
    // Make directory browse-able.
    middlewares.push(connect.directory(directory));
    return middlewares;
  };

  grunt.registerMultiTask('connect', 'Start a connect web server.', function() {
    var done = this.async();
    // Merge task-specific options with these defaults.
    var options = this.options({
      protocol: 'http',
      port: 8000,
      hostname: '0.0.0.0',
      base: '.',
      directory: null,
      keepalive: false,
      debug: false,
      livereload: false,
      open: false,
      useAvailablePort: false,
      onCreateServer: null,
      // if nothing passed, then is set below 'middleware = createDefaultMiddleware.call(this, connect, options);'
      middleware: null
    });

    if (options.protocol !== 'http' && options.protocol !== 'https') {
      grunt.fatal('protocol option must be \'http\' or \'https\'');
    }

    // Connect requires the base path to be absolute.
    if (Array.isArray(options.base)) {
      options.base = options.base.map(function(base) {
        return path.resolve(base);
      });
    } else {
      options.base = path.resolve(options.base);
    }

    // Connect will listen to all interfaces if hostname is null.
    if (options.hostname === '*') {
      options.hostname = '';
    }

    // Connect will listen to ephemeral port if asked
    if (options.port === '?') {
      options.port = 0;
    }

    if (options.onCreateServer && !Array.isArray(options.onCreateServer)) {
      options.onCreateServer = [options.onCreateServer];
    }

    //  The middleware options may be null, an array of middleware objects,
    //  or a factory function that creates an array of middleware objects.
    //  * For a null value, use the default array of middleware
    //  * For a function, include the default array of middleware as the last arg
    //    which enables the function to patch the default middleware without needing to know
    //    the implementation of the default middleware factory function
    var middleware;
    if (options.middleware instanceof Array) {
      middleware = options.middleware;
    } else {
      middleware = createDefaultMiddleware.call(this, connect, options);

      if (typeof(options.middleware) === 'function') {
        middleware = options.middleware.call(this, connect, options, middleware);
      }
    }

    // If --debug was specified, enable logging.
    if (grunt.option('debug') || options.debug === true) {
      connect.logger.format('grunt', ('[D] server :method :url :status ' +
        ':res[content-length] - :response-time ms').magenta);
      middleware.unshift(connect.logger('grunt'));
    }

    // Start server.
    var taskTarget = this.target;
    var keepAlive = this.flags.keepalive || options.keepalive;

    async.waterfall([
      // find a port for livereload if needed first
      function(callback){

        // Inject live reload snippet
        if (options.livereload !== false) {
          if (options.livereload === true) {
            options.livereload = 35729;
          }

          // TODO: Add custom ports here?
          middleware.unshift(injectLiveReload({port: options.livereload}));
          callback(null);
        } else {
          callback(null);
        }
      },
      function(){

        var app = connect.apply(null, middleware);
        var server = null;

        if (options.protocol === 'https') {
          server = https.createServer({
            key: options.key || grunt.file.read(path.join(__dirname, 'certs', 'server.key')).toString(),
            cert: options.cert || grunt.file.read(path.join(__dirname, 'certs', 'server.crt')).toString(),
            ca: options.ca || grunt.file.read(path.join(__dirname, 'certs', 'ca.crt')).toString(),
            passphrase: options.passphrase || 'grunt',
          }, app);
        } else {
          server = http.createServer(app);
        }

        // Call any onCreateServer functions that are present
        if (options.onCreateServer) {
          options.onCreateServer.forEach(function(func) {
            func.call(null, server, connect, options);
          });
        }

        portscanner.findAPortNotInUse(options.port, options.port + MAX_PORTS, options.hostname, function(error, foundPort) {
          // if the found port doesn't match the option port, and we are forced to use the option port
          if (options.port !== foundPort && options.useAvailablePort === false) {
            grunt.fatal('Port ' + options.port + ' is already in use by another process.');
          }

          server
            .listen(foundPort, options.hostname)
            .on('listening', function() {
              var address = server.address();
              var hostname = options.hostname || '0.0.0.0';
              var target = options.protocol + '://' + hostname + ':' + address.port;

              grunt.log.writeln('Started connect web server on ' + target);
              grunt.config.set('connect.' + taskTarget + '.options.hostname', hostname);
              grunt.config.set('connect.' + taskTarget + '.options.port', address.port);

              grunt.event.emit('connect.' + taskTarget + '.listening', hostname, address.port);

              if (options.open === true) {
                open(target);
              } else if (typeof options.open === 'object') {
                options.open.target = options.open.target || target;
                options.open.appName = options.open.appName || null;
                options.open.callback = options.open.callback || function() {};
                open(options.open.target, options.open.appName, options.open.callback);
              } else if (typeof options.open === 'string') {
                open(options.open);
              }

              if (!keepAlive) {
                done();
              }
            })
            .on('error', function(err) {
              if (err.code === 'EADDRINUSE') {
                grunt.fatal('Port ' + foundPort + ' is already in use by another process.');
              } else {
                grunt.fatal(err);
              }
            });
        });

        // So many people expect this task to keep alive that I'm adding an option
        // for it. Running the task explicitly as grunt:keepalive will override any
        // value stored in the config. Have fun, people.
        if (keepAlive) {
          // This is now an async task. Since we don't call the "done"
          // function, this task will never, ever, ever terminate. Have fun!
          grunt.log.write('Waiting forever...\n');
        }
      }
    ]);
  });
};
