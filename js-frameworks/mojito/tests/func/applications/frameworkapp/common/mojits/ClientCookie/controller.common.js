/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('ClientCookie', function(Y, NAME) {

/**
 * The ClientCookie module.
 *
 * @module ClientCookie
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
         *
         * @param ac {Object} The action context that provides access
         *        to the Mojito API.
         */
        index: function(ac) {
            ac.done();
        },

        setCookies: function(ac) {
            var opts = {
                domain: 'mojito-cookie-test.edu',
                path: '/',
                expires: new Date(2035, 1, 1)
            };
            ac.cookie.set('cookie1', 'BUTTER', opts);
            ac.cookie.set('cookie2', 'Chocolate Chips', opts);
            ac.done();
        },
        
        catch: function(ac) {
            var cookies = ac.cookie.get(),
                cookieArray = [];
            Y.Object.each(cookies, function(v, k) {
                cookieArray.push({key: k, value: v});
            });
            ac.done({cookies: cookieArray});
        }

    };

}, '0.0.1', {requires: ['mojito', 'mojito-cookie-addon']});
