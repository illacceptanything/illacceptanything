var wd = require('wd');

var SauceLauncher = function(args, sauceConnect, /* config.sauceLabs */ config, logger, helper,
    baseLauncherDecorator, captureTimeoutLauncherDecorator, retryLauncherDecorator,
    /* sauce:jobMapping */ jobMapping) {

  baseLauncherDecorator(this);
  captureTimeoutLauncherDecorator(this);
  retryLauncherDecorator(this);

  config = config || {};

  var username = args.username || config.username || process.env.SAUCE_USERNAME;
  var accessKey = args.accessKey || config.accessKey || process.env.SAUCE_ACCESS_KEY;
  var tunnelIdentifier = args.tunnelIdentifier || config.tunnelIdentifier;
  var browserName = args.browserName + (args.version ? ' ' + args.version : '') +
                    (args.platform ? ' (' + args.platform + ')' : '');
  var startConnect = config.startConnect !== false;
  var log = logger.create('launcher.sauce');
  var driverLog = logger.create('wd');

  var self = this;
  var driver = wd.promiseChainRemote('ondemand.saucelabs.com', 80, username, accessKey);
  driver.on('status', function(info) {
    driverLog.debug(info.cyan);
  });
  driver.on('command', function(eventType, command, response) {
    driverLog.debug(' > ' + eventType.cyan, command, (response || '').grey);
  });
  driver.on('http', function(meth, path, data) {
    driverLog.debug(' > ' + meth.magenta, path, (data || '').grey);
  });

  var pendingCancellations = 0;
  var sessionIsReady = false;

  if (startConnect && !tunnelIdentifier) {
    tunnelIdentifier = 'karma' + Math.round(new Date().getTime() / 1000);
  }

  var connectOptions = config.connectOptions || {};
  connectOptions = helper.merge(connectOptions, {
    username: username,
    accessKey: accessKey,
    tunnelIdentifier: tunnelIdentifier
  });

  this.name = browserName + ' on SauceLabs';

  var formatSauceError = function(err) {
    return err.message + '\n' + (err.data ? '  ' + err.data : '');
  };

  var pendingHeartBeat;
  var heartbeat = function() {
    pendingHeartBeat = setTimeout(function() {
      log.debug('Heartbeat to Sauce Labs (%s) - fetching title', browserName);
      driver.title().then(null, function(err) {
        log.error('Heartbeat to %s failed\n  %s', browserName, formatSauceError(err));
        clearTimeout(pendingHeartBeat);
        return self._done('failure');
      });
      heartbeat();
    }, 60000);
  };

  var start = function(url) {
    var options = helper.merge(config.options, args, {
      browserName: args.browserName,
      version: args.version || '',
      platform: args.platform || 'ANY',
      tags: args.tags || config.tags || [],
      name: args.testName || config.testName || 'Karma test',
      'tunnel-identifier': tunnelIdentifier,
      'record-video': args.recordVideo || config.recordVideo || false,
      'record-screenshots': (args.recordScreenshots === false || config.recordScreenshots === false) ? false : true,
      'build': args.build || config.build || process.env.TRAVIS_BUILD_NUMBER ||
              process.env.BUILD_NUMBER || process.env.BUILD_TAG ||
              process.env.CIRCLE_BUILD_NUM || null,
      'device-orientation': args.deviceOrientation || null,
      'disable-popup-handler': true
    });

    // Adding any other option that was specified in args, but not consumed from above
    // Useful for supplying chromeOptions, firefoxProfile, etc.
    for (var key in args){
      if (typeof options[key] === 'undefined') {
        options[key] = args[key];
      }
    }

    driver
      .init(options)
      .then(
        function() {
          if (pendingCancellations > 0) {
            pendingCancellations--;
            return;
          }
          // Record the job details, so we can access it later with the reporter
          jobMapping[self.id] = {
            jobId: driver.sessionID,
            credentials: {
              username: username,
              password: accessKey
            }
          };

          sessionIsReady = true;

          log.info('%s session at https://saucelabs.com/tests/%s', browserName, driver.sessionID);
          log.debug('WebDriver channel for %s instantiated, opening %s', browserName, url);
          return driver.get(url).then(heartbeat, function(err) {
            log.error('Can not start %s\n  %s', browserName, formatSauceError(err));
            return self._done('failure');
          });
        }, function(err) {
          if (pendingCancellations > 0) {
            pendingCancellations--;
            return;
          }
          log.error('Can not start %s\n  %s', browserName, formatSauceError(err));
          return self._done('failure');
        }
      ).done();
  };

  this.on('start', function(url) {
    if (pendingCancellations > 0) {
      pendingCancellations--;
      return;
    }

    if (startConnect) {
      sauceConnect.start(connectOptions).then(function() {
        if (pendingCancellations > 0) {
          pendingCancellations--;
          return;
        }

        start(url);
      }, function(err) {
        pendingCancellations--;
        log.error('Can not start %s\n  Failed to start Sauce Connect:\n  %s', browserName, err.message);
        self._retryLimit = -1; // don't retry
        self._done('failure');
      });
    } else {
      start(url);
    }
  });

  this.on('kill', function(done) {
    var allDone = function() {
      self._done();
      done();
    };

    if (sessionIsReady) {
      if (pendingHeartBeat) {
        clearTimeout(pendingHeartBeat);
      }

      log.debug('Shutting down the %s driver', browserName);
      // workaround - navigate to other page to avoid re-connection
      driver.get('about:blank').catch().quit().nodeify(allDone);
      sessionIsReady = false;
    } else {
      pendingCancellations++;
      process.nextTick(allDone);
    }
  });
};

module.exports = SauceLauncher;
