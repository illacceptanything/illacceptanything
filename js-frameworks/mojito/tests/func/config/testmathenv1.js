/*
 * This is a basic func test for a Serveronly application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
     var suite = new Y.Test.Suite("Config: MathEnv1");

     suite.add(new Y.Test.Case({
	  "test MathEnv1": function(){
          Y.Assert.areEqual('This is from application.json:environment:testing', Y.one('#myenv').get('innerHTML'));
          Y.Assert.areEqual('This is from application.json:environment:testing,subject:math', Y.one('#mysubject').get('innerHTML'));
          Y.Assert.areEqual('This is from defaults.json:master', Y.one('#mylang').get('innerHTML'));
      }
  }));

  Y.Test.Runner.add(suite);
});
