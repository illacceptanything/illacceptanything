YUI.add('ActionContextMojit', function(Y, NAME) {

    Y.namespace('mojito.controllers')[NAME] = {

        /**
         * Method corresponding to the 'index' action.
         *
         * @param ac {Object} The action context that provides access
         *        to the Mojito API.
         */
        index: function(ac){
		     ac.done();
		},
        acMojit: function(ac) {
            var test = ac.params.getFromUrl('test');

            var greetingstring = "Hello Action Context Testing";
            var data1 = {
                greeting:greetingstring
            }
            var myCars=new Array("Saab","Volvo","BMW");
            var data2 = {
                mycars:myCars
            }

            if(test=="done1"){
                ac.done(data1);
            }else if(test=="flush1"){
                ac.flush(data1);
                ac.done();
            }else if(test=="done2"){
                ac.done(data1, 'json');
            }else if(test=="flush2"){
                ac.flush(data1, 'json');
                ac.done();
            }else if(test=="done3"){
                ac.done(data1, {name: "json"});
            }else if(test=="flush3"){
                ac.flush(data1, {name: "json"});
                ac.done();
            }else if(test=="done4"){
                ac.done(data2);
            }else if(test=="flush4"){
                ac.flush(data2);
                ac.done();
            }else if(test=="done5"){
                ac.done(data2, 'json');
            }else if(test=="flush5"){
                ac.flush(data2, 'json');
                ac.done();
            }else if(test=="done6"){
                ac.done({data:"Hello, world!--from done"});
            }else if(test=="flush6"){
                ac.flush({data:"Hello, world!--from flush"});
                ac.done();
            }else if(test=="done7"){
                ac.done({data:"Hello, world!"}, 'json');
            }else if(test=="flush7"){
                ac.flush({data:"Hello, world!"}, 'json');
                ac.done();
            }else if(test=="done8"){
                ac.done({data:"Hello, world!--from done"}, {view: {name: "mytemplate"}});
            }else if(test=="done9"){
                ac.done({ foo: null }, {view: {name: "testdir/mytemplate1"}});
            }else if(test=="done10"){
                ac.done({ foo: [ 1, 2, null, 4 ]}, {view: {name: "testdir/mytemplate1"}} );
            }else if(test=="flush8"){
                ac.flush({data:"Hello, world!--from flush"}, {view: {name: "mytemplate"}});
                ac.done();
            }else{
                ac.flush("Hello, world!--from flush,");
                ac.done("Hello, world!--from done");
            }
        }

    };


}, '0.0.1', {requires: ['mojito', 'mojito-params-addon']});
