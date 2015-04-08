// SauceLabs is a wrapper around the Sauce Labs REST API
var SauceLabs = require('saucelabs');

var SauceReporter = function(baseReporterDecorator, logger, /* sauce:jobMapping */ jobMapping) {
  var log = logger.create('reporter.sauce');

  baseReporterDecorator(this);

  var pendingUpdates = 0;
  var updatesFinished = function() {};

  // We're only interested in the final results per browser
  this.onBrowserComplete = function(browser) {
    var result = browser.lastResult;

    // browser.launchId was used until v0.10.2, but changed to just browser.id in v0.11.0
    var browserId = browser.launchId || browser.id;

    if(browserId in jobMapping) {
      var jobDetails = jobMapping[browserId];

      var sauceApi = new SauceLabs(jobDetails.credentials);

      // We record pass/fail status, as well as the full results in "custom-data".
      var payload = {
        passed: !(result.failed || result.error),
        'custom-data': result
      };

      pendingUpdates++;

      sauceApi.updateJob(jobDetails.jobId, payload, function(err) {
        pendingUpdates--;
        if (err) {
          log.error('Failed record pass/fail status: %s', err.error);
        }

        if (pendingUpdates == 0) {
          updatesFinished();
        }
      });
    }
  };

  // Wait until all updates have been pushed to SauceLabs
  this.onExit = function(done) {
    if (pendingUpdates) {
      updatesFinished = done;
    } else {
      done();
    }
  };
};


module.exports = SauceReporter;
