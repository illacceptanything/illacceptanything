/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('DepCheckParent', function(Y, NAME) {

    Y.namespace('mojito.controllers')[NAME] = {
        'index': function(ac) {
            ac.done();
        },
        'mytest': function(ac) {
            //ac.models.AppLevelMojit.getData(function(mydata){
            ac.models.get('model').getData(function(mydata) {
                Y.log("data....." + mydata);
                ac.done({
                    data: mydata
                });
            });
        },
        'metachild': function(ac) {
            ac.composite.done({
                title: 'My Child Mojits:'
            });
        },
        'retrievedata': function(ac) {
            //var retrieved = "old data";
            var retrieved;
            Y.log("data.....");
            ac.meta.retrieve(function(data) {
                Y.log("data1.....");
                retrieved = data["mydata"];
                Y.log("data....." + retrieved);
                ac.done({
                    retrieved: "any....data"
                });
            });
            // ac.done({retrieved:retrieved});
        }
    };

}, '0.0.1', {requires: [
    'mojito',
    'mojito-composite-addon',
    'mojito-models-addon',
    'mojito-intl-addon',
    'mojito-util',
    'querystring-stringify',
    //'AppLevelMojitModel',
    'DepCheckModel',
    'mojito-meta-addon']});
