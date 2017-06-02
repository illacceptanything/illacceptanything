/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('BroadCast', function(Y, NAME) {

/**
 * The Parent module.
 *
 * @module Parent
 */

    /**
     * Constructor for the Controller class.
     *
     * @class Controller
     * @constructor
     */
    Y.namespace('mojito.controllers')[NAME] = {

        'index': function(ac) {
            ac.done();
        },

        'static': function(ac) {
            ac.assets.addCss('/static/BroadCast/assets/static.css');
            ac.composite.done();
        },

        'destroychild': function(ac) {
            Y.log("inside destroy controller");
            ac.assets.addCss('/static/BroadCast/assets/static.css');
            ac.composite.done();
        },

        'dyno': function(ac) {

            var children = {
                    children: {
                    }
                },
                startAt = Math.ceil(Math.random()*1000);

            function attach(type, no) {
                no = no || 0;
                children.children['' + startAt++] = {
                    type: type,
                    config: {
                        order: no
                    }
                };
            }

            attach('GreenChild', 1);
            attach('GreenChild', 2);
            attach('GreenChild', 3);
            attach('RedChild', 4);
            attach('RedChild', 5);
            attach('RedChild', 6);
            attach('RedChild', 7);
            attach('GreenChild', 8);
            attach('GreenChild', 9);
            attach('BlueChild', 8);
            attach('BlueChild', 9);

            ac.assets.addCss('/static/BroadCast/assets/style.css');

            ac.composite.execute(children, function(data, meta) {
                var template = {childrenHere: ''};
                Y.Object.keys(data).forEach(function(k) {
                    template.childrenHere += data[k];
                });
                ac.done(template, meta);
            });
        },

        'destroydynochild': function(ac) {

            var children = {
                    children: {
                    }
                },
                startAt = Math.ceil(Math.random()*1000);

            function attach(type, no) {
                no = no || 0;
                children.children['' + startAt++] = {
                    type: type,
                    config: {
                        order: no
                    }
                };
            }

            attach('GreenChild', 1);
            attach('GreenChild', 2);
            attach('RedChild', 3);
            attach('RedChild', 4);
            attach('BlueChild', 5);
            attach('BlueChild', 6);

            ac.assets.addCss('/static/BroadCast/assets/style.css');

            ac.composite.execute(children, function(data, meta) {
                var template = {childrenHere: ''};
                Y.Object.keys(data).forEach(function(k) {
                    template.childrenHere += data[k];
                });
                ac.done(template, meta);
            });
        },

        'selfinvoke': function(ac){
            //var foovalue = ac.params.getFromBody('foo');
            //Y.log("Here11111....."+foovalue);
            ac.composite.done({
                title: 'foovalue='
            });
        }

    };

}, '0.0.1', {requires: [
    'mojito',
    'mojito-composite-addon',
    'mojito-assets-addon'
]});
