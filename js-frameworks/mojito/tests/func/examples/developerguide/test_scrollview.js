/*
 * This is a basic func test for a UseCase application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', function (Y) {
   
         var suite = new Y.Test.Suite("DeveloperGuide: scrollview");

         suite.add(new Y.Test.Case({
         
             "test scrollview": function() {
                 Y.Assert.areEqual("Scroll View", Y.one('#scrollview-header h1').get('innerHTML'));
                 Y.Assert.areEqual("yui3-widget yui3-scrollview yui3-scrollview-horiz", Y.one('#scrollview').getAttribute('class'));
                 for(i=0; i<Y.all('#scrollview-content img').size(); i++){
                      Y.Assert.areEqual("static.flickr.com", Y.all('#scrollview-content img').item(i).getAttribute('src').match(/static.flickr.com/gi));
                 }
             }
         }));    

         Y.Test.Runner.add(suite);
});

