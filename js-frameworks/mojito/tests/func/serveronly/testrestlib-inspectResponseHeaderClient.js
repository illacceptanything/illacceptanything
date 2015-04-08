/*
 * This is a basic func test for a Serveronly application.
 */
YUI.add('serveronly-inspectresponseheaderclient-tests', function (Y) {

    var suite = new Y.Test.Suite("ServerOnly: inspectResponseHeaderClient");
    suite.add(new Y.Test.Case({
        "test inspectResponseHeaderClient": function() {
            var i,
                found = false,
                expect = ['Content-Type', 'content-type'];

                Y.Assert.areEqual('Headers:', Y.one('#headers').get('innerHTML').match(/Headers:/gi));
                var result = Y.one('#headers').get('innerHTML').match(/content-type/gi);
                for(i = 0; i< expect.length; i++){
                    if (result == expect[i]) {
                        found = true;
                    }
                }
                Y.Assert.isTrue(found);
                Y.Assert.areEqual('Connection: keep-alive', Y.one('#headers').get('innerHTML').match(/Connection: keep-alive/gi));
                Y.Assert.areEqual('Transfer-Encoding: chunked', Y.one('#headers').get('innerHTML').match(/Transfer-Encoding: chunked/gi));
                Y.Assert.areEqual('Specific Header: text\/html; charset=utf-8', Y.one('#specific_header').get('innerHTML'));
        }
       
    }));
    
    Y.Test.Runner.add(suite);

}, '0.0.1', {requires: [
    'node', 'node-event-simulate', 'test', 'console'
]});
