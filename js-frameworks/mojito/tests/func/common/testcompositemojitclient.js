/*
 * This is a basic func test for a Common application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', 'json-parse', function (Y) {

    var suite = new Y.Test.Suite("Common: compositemojitclient");

    suite.add(new Y.Test.Case({

        "test compositemojitclient": function () {
            var childConfig1 = Y.all('#childrenconfig').item(0).get('text').replace('nav: ', ''),
                childConfig2 = Y.all('#childrenconfig').item(1).get('text').replace('news: ', ''),
                childConfig3 = Y.all('#childrenconfig').item(2).get('text').replace('footer: ', '');

            try {
                childConfig1 = Y.JSON.parse(childConfig1);
                childConfig2 = Y.JSON.parse(childConfig2);
                childConfig3 = Y.JSON.parse(childConfig3);
            } catch (e) {
                this.fail(e);
            }

            Y.Assert.areEqual('This is the title from controller.js of layout', Y.one('#cmtitle').get('innerHTML').match(/This is the title from controller.js of layout/gi));
            Y.Assert.areEqual('Info about all the children:', Y.one('#cmtitle3').get('innerHTML').match(/Info about all the children:/gi));

            Y.Assert.isObject(childConfig1);
            Y.Assert.areEqual('CM_Nav', childConfig1.type);
            Y.Assert.areEqual('nav', childConfig1.config.id);
            Y.Assert.isNotUndefined('nav', childConfig1.instanceId, 'instanceId for CM_Nav is undefined');
            Y.Assert.isObject(childConfig2);
            Y.Assert.areEqual('CM_News', childConfig2.type);
            Y.Assert.areEqual('news', childConfig2.config.id);
            Y.Assert.isNotUndefined('nav', childConfig2.instanceId, 'instanceId for CM_News is undefined');
            Y.Assert.isObject(childConfig3);
            Y.Assert.areEqual('CM_Footer', childConfig3.base);
            Y.Assert.areEqual('footer_id', childConfig3.config.id);
            Y.Assert.isNotUndefined('nav', childConfig3.instanceId, 'instanceId for CM_Footer is undefined');
        }

    }));

    Y.Test.Runner.add(suite);

});
