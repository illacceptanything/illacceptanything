/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add('clicklog', function(Y, NAME) {

/**
 * The ClickLog mojit which just shows running log of user clicks.
 * This mojit primarily exists to demonstrate rendering of views
 * in the binder.
 *
 * @module clicklog
 */

    /**
     * @class Controller
     */
    Y.namespace('mojito.controllers')[NAME] = {

        /**
         * This "index" action generates the initial view.
         * The binder attached to this view is what does most of the work
         * of this mojit.
         *
         * @param ac {Object} The action context that provides access
         *        to the Mojito API.
         */
        index: function(ac) {
            ac.assets.addCss('./index.css');
            ac.done({});
        }

    };

}, '0.0.1', {requires: ['mojito', 'mojito-assets-addon']});
