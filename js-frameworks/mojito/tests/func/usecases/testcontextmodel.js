/*
 * This is a basic func test for a UseCase application.
 */
YUI.add('usecases-testcontextmodel-tests', function (Y) {
    var suite = new Y.Test.Suite("UseCases: contextModel");

    suite.add(new Y.Test.Case({
        "test contextualizedmodel": function() {
            if(ARROW.testParams["testName"] === "Default") {
                Y.Assert.areEqual('Regular view', Y.one('#title').get('innerHTML'));
                Y.Assert.areEqual('Hello Mojito', Y.one('#msg').get('innerHTML'));
            } else {
                Y.Assert.areEqual('IPhone view', Y.one('#title').get('innerHTML'));
                Y.Assert.areEqual('Hello Iphone Data', Y.one('#msg').get('innerHTML'));
            }
        }
    }));    

    Y.Test.Runner.add(suite);
}, '0.0.1', {requires: [
    'node', 'node-event-simulate', 'test'
]});

