      YUI({
        useConsoleOutput: true,
        useBrowserConsole: true,
        logInclude: { TestRunner: true }
      }).use('node', 'node-event-simulate', 'test', 'console', function (Y) {

        'use strict';
        var suite = new Y.Test.Suite("Dashboard App: Mojits Module"),
            url = window.location.protocol + "//" + window.location.host + "/";
        suite.add(new Y.Test.Case({
          "test heading of page and GitHub section": function () {
            // Tests the title in HTML header
            Y.Assert.areEqual("Trib - YUI/Mojito Developer Dashboard", Y.one('div h4').get('innerHTML'));

            // Tests the title within the content
            Y.Assert.areEqual("YUI GitHub Stats", Y.one('div h3').get('innerHTML'));
          }
        }));
        Y.Test.Runner.add(suite);
      });
