/*
 * This is a basic func test for a Serveronly application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
     var suite = new Y.Test.Suite("Config: DefaultConfig2");

     suite.add(new Y.Test.Case({
	  "test DefaultConfig2": function(){
          Y.Assert.areEqual('This is from defaults.json:runtime:server', Y.one('#myenv').get('innerHTML'));
          Y.Assert.areEqual('This is from defaults.json:master', Y.one('#mysubject').get('innerHTML'));
          Y.Assert.areEqual('This is from defaults.json:lang:de', Y.one('#mylang').get('innerHTML'));
      }
  }));

  Y.Test.Runner.add(suite);
});
