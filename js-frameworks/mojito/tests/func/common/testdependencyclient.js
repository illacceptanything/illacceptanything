/*
 * This is a basic func test for a Common application.
 */
YUI.add('testdependencyclient-tests', function (Y) {
   
    var suite = new Y.Test.Suite("Common: dependencyclient");

    suite.add(new Y.Test.Case({

        "test dependencyclient": function() {
            Y.Assert.areEqual('0,1,Aardvark,attic,zebra,Zoo', Y.one('#myarray').get('innerHTML').match(/0,1,Aardvark,attic,zebra,Zoo/gi));
        }

    }));

     Y.Test.Runner.add(suite);

}, '0.0.1', { requires: ['node', 'node-event-simulate', 'test']});