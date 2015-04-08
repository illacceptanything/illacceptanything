   YUI({
        useConsoleOutput: true,
        useBrowserConsole: true,
        logInclude: { TestRunner: true }
        }).use('node', 'node-event-simulate', 'test', 'console', function (Y) {

        'use strict';
        var suite = new Y.Test.Suite("TribApp: Localization Test"),
            url = window.location.protocol + "//" + window.location.host + "/";
        suite.add(new Y.Test.Case({
            "test en-US title": function () {
                // Tests the title within the content
                Y.Assert.areEqual("Trib - YUI Developer Dashboard", Y.one('body h1').get('innerHTML'));
            }
        }));
        Y.Test.Runner.add(suite);
    });
