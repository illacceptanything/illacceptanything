/*
 * This is a basic func test for a Common application.
 */
YUI.add('common-testlazyloadrpcclient-tests', function (Y) {
   
    var suite = new Y.Test.Suite("Common: LazyLoadRPCClient");

    suite.add(new Y.Test.Case({

        "test LazyLoadRPCClient": function() {
            Y.Assert.areEqual('Lazy load succeeded:', Y.one('#LazyLoadRPCtitle').get('innerHTML'));
            Y.Assert.areEqual('foo-value set by binder', Y.one('#LazyLoadRPCfoo').get('innerHTML'));
            Y.Assert.areEqual('bar-value set by controller', Y.one('#LazyLoadRPCbar').get('innerHTML'));
            Y.Assert.areEqual('From controller: foo-value set by binder', Y.one('#LazyLoadRPCbaz').get('innerHTML'));
            Y.Assert.areEqual('Data has changed: 2 times', Y.one('#LazyLoadRPCcount').get('innerHTML'));
        }
    }));
    
    Y.Test.Runner.add(suite);

}, '0.0.1', { requires: [
    'node', 'node-event-simulate', 'test', 'console'
]});

