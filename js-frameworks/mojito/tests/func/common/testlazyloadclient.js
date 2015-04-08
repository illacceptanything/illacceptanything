/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', 'json-parse', function (Y) {
   
    var suite = new Y.Test.Suite("Common: lazyloadclient");

    suite.add(new Y.Test.Case({

        "test lazyloadclient": function() {
            var finalLazyResultText = Y.one('#finalLazyResult').get('text'),
                finalLazyResult;
            try {
                    finalLazyResult = Y.JSON.parse(finalLazyResultText);
            } catch (e) {
                    Y.Assert.isTrue(false, 'Failed to parse JSON: ' + finalLazyResultText);
            }
            Y.Assert.areEqual('Lazy Loading', Y.one('#header1').get('innerHTML').match(/Lazy Loading/gi));
            Y.Assert.areEqual('Defer:true', Y.one('#header2').get('innerHTML').match(/Defer:true/gi));
            Y.Assert.areEqual('I was lazy-loaded', Y.one('#hello').get('innerHTML').match(/I was lazy-loaded/gi));

            Y.Assert.isObject(finalLazyResult.mojit);
            Y.Assert.areEqual('LazyChild', finalLazyResult.mojit.type);
            Y.Assert.areEqual('hello', finalLazyResult.mojit.action);
            Y.Assert.isFalse(finalLazyResult.mojit.defer);

            Y.Assert.areEqual('LAZY LOAD COMPLETE', Y.one('#lazyResult').get('innerHTML').match(/LAZY LOAD COMPLETE/gi));
        }

    }));

    Y.Test.Runner.add(suite);

});