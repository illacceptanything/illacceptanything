/*
 * This is a basic func test for a UseCase application.
 */
YUI.add('usecases-testi18nclient-tests', function (Y) {
    var suite = new Y.Test.Suite("UseCases: i18n-3");

    suite.add(new Y.Test.Case({
        "test i18n client": function() {
            var lang = this.testParams["lang"];

            Y.Assert.areEqual('status', Y.one('#parent_status').get('innerHTML'));
            Y.Assert.areEqual('Mojito is working.', Y.one('#parent_status_value').get('innerHTML'));
            Y.Assert.areEqual('data', Y.one('#parent_data').get('innerHTML'));
            Y.Assert.areEqual('child', Y.one('#child').get('innerHTML'));
            Y.Assert.areEqual('status', Y.one('#child_status').get('innerHTML'));
            Y.Assert.areEqual('Mojito child is working.', Y.one('#child_status_value').get('innerHTML'));
            Y.Assert.areEqual('data', Y.one('#child_data').get('innerHTML'));
            if (lang === "en") {
                Y.Assert.areEqual('some: master en', Y.one('#parent_data_value').get('innerHTML').match(/some: master en/gi));
                Y.Assert.areEqual('some: child en', Y.one('#child_data_value').get('innerHTML').match(/some: child en/gi));
            } else {
                Y.Assert.areEqual('some: master zh', Y.one('#parent_data_value').get('innerHTML').match(/some: master zh/gi));
                Y.Assert.areEqual('some: child zh', Y.one('#child_data_value').get('innerHTML').match(/some: child zh/gi));
            }

        }
    }));

    Y.Test.Runner.add(suite);
}, '0.0.1', {requires: [
    'node', 'node-event-simulate', 'test', 'console', 'intl', 'datatype-date-format'
]});

