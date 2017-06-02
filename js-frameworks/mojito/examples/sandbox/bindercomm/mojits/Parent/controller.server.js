/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add('parent', function(Y, NAME) {

/**
 * The parent module.
 *
 * @module parent
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
            ac.assets.addCss('/static/Parent/assets/static.css');
            ac.composite.done();
        },

        dyno: function(ac) {

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

            attach('RedChild', 1);
            attach('RedChild', 2);
            attach('RedChild', 3);
            attach('RedChild', 4);
            attach('RedChild', 5);
            attach('RedChild', 6);
            attach('RedChild', 7);
            attach('RedChild', 8);
            attach('RedChild', 9);

            ac.assets.addCss('/static/Parent/assets/style.css');

            ac.composite.execute(children, function(data, meta) {
                var template = {childrenHere: ''};
                Object.keys(data).forEach(function(k) {
                    template.childrenHere += data[k];
                });
                ac.done(template, meta);
            });
        },

        dummy: function(ac) {
            ac.done({status: 'done'}, 'json');
        }

    };

}, '0.0.1', {requires: ['mojito-composite-addon', 'mojito-assets-addon']});
