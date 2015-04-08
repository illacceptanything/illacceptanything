/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint anon:true, sloppy:true, nomen:true*/

YUI.add('simple', function (Y, NAME) {
    /** 
    * The simple module. 
    * 
    * @module simple 
    */
    /**     
    * Constructor for the Controller class.     
    *
    * @class Controller     
    * @constructor     
    */
    Y.namespace('mojito.controllers')[NAME] = {
        /**
        * Method corresponding to the 'index' action.
        * @param ac {Object} The action context that 
        * provides access to the Mojito API.         
        */
        index: function(ac) {
            var data = {
                title: "Simple Assets",
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
