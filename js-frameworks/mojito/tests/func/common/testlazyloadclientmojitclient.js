/*
 * This is a basic func test for a Common application.
 */
YUI.add('common-testlazyloadclientmojitclient-tests', function (Y) {
   
    var suite = new Y.Test.Suite("Common: LazyLoadClientMojitCommon");

    suite.add(new Y.Test.Case({

        "test LazyLoadClientMojitClient": function() {
            Y.Assert.areEqual('Lazy load succeeded:', Y.one('#LazyLoadClientMojittitle').get('innerHTML'));
            Y.Assert.areEqual('fooc-value set by binder', Y.one('#LazyLoadClientMojitfoo').get('innerHTML'));
            Y.Assert.areEqual('barc-value set by controller', Y.one('#LazyLoadClientMojitbar').get('innerHTML'));
            Y.Assert.areEqual('From controller: fooc-value set by binder', Y.one('#LazyLoadClientMojitbaz').get('innerHTML'));
            Y.Assert.areEqual('Data has changed: 3 times', Y.one('#LazyLoadClientMojitcount').get('innerHTML'));
        }

    }));
    Y.Test.Runner.add(suite);

}, '0.0.1', { requires: [
    'node', 'node-event-simulate', 'test', 'console'
]});

