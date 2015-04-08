/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint anon:true, sloppy:true, nomen:true*/

YUI.add('framed', function (Y, NAME) {
    Y.namespace('mojito.controllers')[NAME] = {
        index: function (ac) {
            var data = {
                title: "Framed Assets",
                colors: [
                    {id: "green", rgb: "#616536"},
                    {id: "brown", rgb: "#593E1A"},
                    {id: "grey",  rgb: "#777B88"},
                    {id: "blue",  rgb: "#3D72A4"},
                    {id: "red",  rgb: "#990033"}
                ]
            };
            ac.done(data);
        }
    };
}, '0.0.1', {requires: []});
