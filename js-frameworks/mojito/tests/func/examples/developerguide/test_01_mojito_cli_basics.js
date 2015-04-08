      YUI({
        useConsoleOutput: true,
        useBrowserConsole: true,
        logInclude: { TestRunner: true }
      }).use('node', 'node-event-simulate', 'test', 'console', function (Y) {

        'use strict';
        var suite = new Y.Test.Suite("Dashboard App: 01 CLI Basics App"),
            url = window.location.protocol + "//" + window.location.host + "/";
        suite.add(new Y.Test.Case({
          "test page loads and shows default content": function () {
            // Tests the title in HTML header
            Y.Assert.areEqual("Mojito is working.", Y.one('#dd_status').get('innerHTML'));
          }
        }));
        Y.Test.Runner.add(suite);
      });
