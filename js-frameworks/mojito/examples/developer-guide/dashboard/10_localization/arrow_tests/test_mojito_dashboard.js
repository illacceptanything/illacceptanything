YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {

    'use strict';
    var suite = new Y.Test.Suite("TribApp: Mojito Dashboard test"),
        url = window.location.protocol + "//" + window.location.host + "/";
    suite.add(new Y.Test.Case({
        "test Mojito dashboard": function () {
            // Tests the title in HTML header
            Y.Assert.areEqual("Trib - YUI/Mojito Developer Dashboard", Y.one('head title').get('innerHTML'));

            // Tests the  YUI button
            Y.Assert.areEqual(url, Y.one('a.yui3-button.swap').get('href'));

            // Tests the title within the content
            Y.Assert.areEqual("Trib - Mojito Developer Dashboard", Y.one('body h1').get('innerHTML'));
        }
    }));
    Y.Test.Runner.add(suite);
});
