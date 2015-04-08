/*
 * This is a basic func test for a UseCase application.
 */
YUI.add('usecases-testi18nintLang-tests', function (Y) {
    var suite = new Y.Test.Suite("UseCases: i18nIntlLang");

    suite.add(new Y.Test.Case({
        "test i18n int lang": function() {
            Y.Assert.areEqual('Pick your order', Y.one('h3').get('innerHTML'));
            Y.Assert.areEqual('First Choice Mojito', Y.one('#order1').get('innerHTML'));
            Y.Assert.areEqual('Second Choice Bronx', Y.one('#order2').get('innerHTML'));
            Y.Assert.areEqual('Third Choice Zombie and Earthquake', Y.one('#order3').get('innerHTML'));
        }
    }));

    Y.Test.Runner.add(suite);
}, '0.0.1', {requires: [
    'node', 'node-event-simulate', 'test', 'console', 'intl', 'datatype-date-format'
]});
