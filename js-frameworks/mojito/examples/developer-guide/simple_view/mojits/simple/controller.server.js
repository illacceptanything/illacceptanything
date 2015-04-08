/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint anon:true, sloppy:true, nomen:true*/

YUI.add('simple', function (Y, NAME) {
    Y.namespace('mojito.controllers')[NAME] = {
        index: function(ac) {
            var today = new Date(),
                hours = today.getHours(),
                h = hours > 12 ? hours % 12 : hours;
                data = {
                    type : 'simple',
                    time : { hours: h, minutes: today.getMinutes() < 10 ? "0" + today.getMinutes() : today.getMinutes(), period: today.getHours() >= 12 ? "p.m." : "a.m."},
                    show : true,
                    hide : false,
                    list : [{id: 2}, {id: 1}, {id: 3} ],
                    hole : null,
                    html : "<h3 style='color:red;'>simple html</h3>"
                };
            ac.done(data);
        }
    };
}, '0.0.1', {requires: []});

