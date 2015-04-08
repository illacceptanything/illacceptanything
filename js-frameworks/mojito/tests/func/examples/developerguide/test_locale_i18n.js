/*
 * This is a basic func test for a UseCase application.
 */
YUI({
    useConsoleOutput: true,
    useBrowserConsole: true,
    logInclude: { TestRunner: true }
}).use('node', 'node-event-simulate', 'test', 'console', 'intl', 'datatype-date-format', function (Y) {
   
         var suite = new Y.Test.Suite("DeveloperGuide: locale_i18n");

         suite.add(new Y.Test.Case({
         
             "test locale_i18n": function() {
                 var test = ARROW.testParams["testName"];
                     body = Y.one('body').get('innerHTML');
                     actualdate = body.substr(body.indexOf('-')+3, 5);
                     date = new Date();
                     UTCDate = new Date();
                 UTCDate.setTime(date.getTime()+date.getTimezoneOffset()*60000)

                 if(test === "Default"){
                     Y.log("here...."+Y.one('body').get('innerHTML'));
                     Y.Assert.areEqual("Hello!", Y.one('body').get('innerHTML').match(/Hello!/gi));
                     var expecteddate1 = Y.DataType.Date.format(date, {format:"%m/%d"});
                         expecteddate2 = Y.DataType.Date.format(UTCDate, {format:"%m/%d"});
                     Y.Assert.isTrue(expecteddate1 === actualdate || expecteddate2 == actualdate);
                 }else if(test === "en-AU"){
                     Y.log("here...."+Y.one('body').get('innerHTML'));
                     Y.Assert.areEqual("G'day!", Y.one('body').get('innerHTML').match(/G'day!/gi));
                     var expecteddate1 = Y.DataType.Date.format(date, {format:"%d/%m"});
                         expecteddate2 = Y.DataType.Date.format(UTCDate, {format:"%m/%d"});
                     Y.Assert.isTrue(expecteddate1 === actualdate || expecteddate2 == actualdate);
                 }else if(test === "fr-FR"){
                     Y.log("here...."+Y.one('body').get('innerHTML'));
                     Y.Assert.areEqual("Tiens!", Y.one('body').get('innerHTML').match(/Tiens!/gi));
                     var expecteddate1 = Y.DataType.Date.format(date, {format:"%d/%m"});
                         expecteddate2 = Y.DataType.Date.format(UTCDate, {format:"%m/%d"});
                     Y.Assert.isTrue(expecteddate1 === actualdate || expecteddate2 == actualdate);
                 }
             }
         }));    

         Y.Test.Runner.add(suite);
});

