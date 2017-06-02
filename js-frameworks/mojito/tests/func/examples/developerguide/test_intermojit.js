/*
 * This is a basic func test for a UseCase application.
 */
'use strict';

YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {

    var suite = new Y.Test.Suite("DeveloperGuide: intermojit");

    suite.add(new Y.Test.Case({

        "test intermojit": function () {
            var i;
            if (ARROW.testParams["testName"] === "first") {
                Y.Assert.areEqual("List of images for testing", Y.one('h3').get('innerHTML'));
                for (i = 0; i < Y.all('a').size(); i++) {
                    Y.Assert.areEqual("static.flickr.com", Y.all('a').item(i).getAttribute('href').match(/static.flickr.com/gi));
                    Y.Assert.areEqual("Image", Y.all('a').item(i).get('innerHTML').match(/Image/gi));
                }
            } else {
                Y.Assert.areEqual('Image matching the link clicked on the left.', Y.all('h3').item(1).get('innerHTML'));
                Y.Assert.areEqual("static.flickr.com", Y.one('img').getAttribute('src').match(/static.flickr.com/gi));
            }
        }
    }));
    
    Y.Test.Runner.add(suite);
});

