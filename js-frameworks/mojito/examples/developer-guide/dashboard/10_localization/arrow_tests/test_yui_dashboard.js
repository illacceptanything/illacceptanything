/*
 * Copyright (c) 2013 Yahoo! Inc. All rights reserved.
 */

/**
This is a basic func test for Mojito Trib Application.
**/
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {

    'use strict';
    var suite = new Y.Test.Suite("TribApp: YUI Dashboard test"),
        url = window.location.protocol + "//" + window.location.host + "/";
    suite.add(new Y.Test.Case({
        "test YUI Dashboard": function () {
            // Tests the title in HTML header
            Y.Assert.areEqual("Trib - YUI/Mojito Developer Dashboard", Y.one('head title').get('innerHTML'));

            // Tests the title within the content
            Y.Assert.areEqual("Trib - YUI Developer Dashboard", Y.one('body h1').get('innerHTML'));
        }
    }));
    Y.Test.Runner.add(suite);
});

