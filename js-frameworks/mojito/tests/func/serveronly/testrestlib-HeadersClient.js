/*
 * This is a basic func test for a Serveronly application.
 */
YUI.add('serveronly-headerclient-tests', function (Y) {

    var suite = new Y.Test.Suite("ServerOnly: HeadersClient");
    suite.add(new Y.Test.Case({
        "test HeadersClient": function() {
            var expect = ['keep-alive', 'Keep-Alive'],
                found = false;
            Y.Assert.areEqual('somevalue', Y.one('#my_header').get('innerHTML'));
            var result = Y.one('#connection').get('innerHTML');
            for(i = 0; i< expect.length; i++){
                if (result === expect[i]) {
                    found = true;
                }
            }
            Y.Assert.isTrue(found);
        }
    }));
    Y.Test.Runner.add(suite);

}, '0.0.1', {requires: [
    'node', 'node-event-simulate', 'test'
]});
