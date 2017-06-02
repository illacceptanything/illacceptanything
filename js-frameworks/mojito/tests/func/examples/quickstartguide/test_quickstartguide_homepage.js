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

This is a basic func test for Mojito QuickStartGuide.
**/
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
        var suite = new Y.Test.Suite("QuickStartGuide: Homepage test");

        suite.add(new Y.Test.Case({

            "test Homepage title": function() {
                // Tests the title in HTML header
                Y.Assert.areEqual("Mojito Quickstart Guide", Y.one('head title').get('innerHTML'));

                // Tests the Mojito banner link
                Y.Assert.areEqual("http://developer.yahoo.com/cocktails/mojito/", Y.one('.container .logo-nav .mojito_logo').get('href'))

                // Tests the title within the content
                Y.Assert.areEqual("Quickstart Guide", Y.one('.container .logo-nav h1').get('innerHTML'));
            },

            "test Homepage description link": function() {
                // Tests the link to the correct git repo address
                Y.Assert.areEqual("https://github.com/yahoo/mojito/tree/develop/examples/quickstartguide", Y.one('.container .description a').get('href'));
            }
        }));

    Y.Test.Runner.add(suite);
});

