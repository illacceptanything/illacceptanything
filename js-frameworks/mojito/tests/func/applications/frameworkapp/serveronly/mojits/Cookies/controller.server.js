/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('Cookies', function(Y, NAME) {

/**
 * The Cookies module.
 *
 * @module Cookies
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
         * @param ac {Object} The ActionContext that provides access
         *        to the Mojito API.
         */
        index: function(ac) {
            ac.done();
        },

        showCookies: function(ac) {
            var cookies = ac.cookie.get(),
                line = '',
                output = '';

            for (var name in cookies) {
                line = name + '=' + cookies[name] + '<p>';
                output += line;
            }

            ac.done(output);
        }

    };

}, '0.0.1', {requires: [
    'mojito',
    'mojito-cookie-addon']});
