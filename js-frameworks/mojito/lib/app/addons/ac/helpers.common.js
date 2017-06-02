/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint nomen:true*/
/*global YUI*/

/**
 * @module ActionContextAddon
 */
YUI.add('mojito-helpers-addon', function (Y, NAME) {

    'use strict';

    /**
     * <strong>Access point:</strong> <em>ac.helpers.*</em>
     * Addon that provides helpers functionalities
     * @class Helpers.common
     */
    function Addon(command, adapter) {
        this._page = adapter.page;
        // exposing instance helpers for render engine can use it,
        // also, mixing in any previously exposed helper.
        this._helpers = command.instance.helpers = Y.merge({},
            (this._page.helpers || {}));
    }

    Addon.prototype = {

        namespace: 'helpers',

        /**
         * Gets one specific helper if the name is specified,
         * otherwise returns all available helpers.
         * @method get
         * @param {string} helperName The optional helper name
         * @return {function|object} a helper function or all available helpers
         */
        get: function (helperName) {
            return helperName ? this._helpers[helperName] : this._helpers;
        },

        /**
         * set a helper function at the mojit instance.
         * @method set
         * @param {string} helperName The helper name.
         * @param {function} helper The helper function.
         */
        set: function (helperName, helper) {
            if (!helperName || !helper) {
                Y.log('Invalid helper name or helper function ' +
                    'when calling `ac.helpers.set()`: ' + helperName, 'error', NAME);
                return;
            }
            if (this._helpers[helperName]) {
                Y.log('Overiding an existing helper function with name: ' +
                    helperName, 'warn', NAME);
            }
            this._helpers[helperName] = helper;
        },

        /**
         * Expose a helper function as global. On the server side
         * this means any mojit instance under a particular request
         * can use the helper. On the client, any
         * mojit instance on the page can use the helper.
         * @method expose
         * @param {string} helperName The helper name.
         * @param {function} helper Optional helper function, if not
         * present, the helper will be lookup by name.
         */
        expose: function (helperName, helper) {
            this._page.helpers = this._page.helpers || {};
            // you might want to expose an existing local helper
            helper = helper || this._helpers[helperName];
            // exposing thru global page object
            this._page.helpers[helperName] = helper;
            // exposing at the instance level
            this._helpers[helperName] = helper;
            Y.log('Exposing a global helper: ' + helperName, 'info', NAME);
        }

    };

    Y.namespace('mojito.addons.ac').helpers = Addon;

}, '0.1.0', {requires: [
    'mojito',
    'mojito-util'
]});
