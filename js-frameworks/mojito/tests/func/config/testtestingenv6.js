/*
 * This is a basic func test for a Serveronly application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
     var suite = new Y.Test.Suite("Config: TestingEnv6");

     suite.add(new Y.Test.Case({
	  "test TestingEnv6": function(){
          Y.Assert.areEqual('This is from application.json:environment:testing', Y.one('#myenv').get('innerHTML'));
          Y.Assert.areEqual('This is from defaults.json:master', Y.one('#mysubject').get('innerHTML'));
          Y.Assert.areEqual('This is from application.json:environment:testing,lang:de', Y.one('#mylang').get('innerHTML'));
      }
  }));

  Y.Test.Runner.add(suite);
});
