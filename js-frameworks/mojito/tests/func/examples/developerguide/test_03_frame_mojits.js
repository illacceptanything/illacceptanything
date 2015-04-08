      YUI({
        useConsoleOutput: true,
        useBrowserConsole: true,
        logInclude: { TestRunner: true }
      }).use('node', 'node-event-simulate', 'test', 'console', function (Y) {

        'use strict';
        var suite = new Y.Test.Suite("Dashboard App: Frame Mojit Module"),
            url = window.location.protocol + "//" + window.location.host + "/";
        suite.add(new Y.Test.Case({
          "test HTML title and heading for GitHub mojit": function () {
            // Tests the title in HTML header
            Y.Assert.areEqual("Trib - YUI/Mojito Developer Dashboard", Y.one('head title').get('innerHTML'));


            // Tests the title within the content
            Y.Assert.areEqual("YUI GitHub Stats", Y.one('div h3').get('innerHTML'));
          }
        }));
        Y.Test.Runner.add(suite);
      });
