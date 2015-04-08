/*
 * Copyright (c) 2013 Yahoo! Inc. All rights reserved.
 */

/**
Mojito Built-In Functional/Unit Tests

Mojito comes with the script run.js that allows you to run built-in unit and functional tests. The script 
run.js uses the npm module Arrow, a testing framework that fuses together JavaScript, Node.js, PhantomJS, 
and Selenium. By running the built-in unit and functional tests, contributors can accelerate the merging 
of their pull request.
For more info, visit: http://developer.yahoo.com/cocktails/mojito/docs/topics/mojito_testing.html#func-unit-builtin

This is a functional test for Mojito QuickStartGuide when Reader's page encounters a bad URL.
**/
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {

        var suite = new Y.Test.Suite("QuickStartGuide: Reader error test");

        suite.add(new Y.Test.Case({
            "test Reader nav bar": function() {
                // Tests the previous link, should not present
                Y.Assert.isNull(Y.one('.container .contents-nav .link-prev'));

                // Tests the next link, should not present
                Y.Assert.isNull(Y.one('.container .contents-nav .link-next'));

                // HOME menu
                Y.Assert.areEqual("HOME", Y.one('.container .contents-nav .link-home').get('innerHTML'));

                // Tests error article title
                Y.Assert.areEqual("oh noes!", Y.one('.container .content-title h2').get('innerHTML'));

                // Tests article content
                Y.Assert.areEqual("Ooo, could not fetch content for undefined.md", Y.one('.container .content #desc').get('innerHTML'));

                // Tests error article footer
                Y.Assert.areEqual("oh noes!", Y.one('.container .link-bottom p').get('innerHTML'));
            }
        }));

    Y.Test.Runner.add(suite);
});

