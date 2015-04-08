/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add('container-binder-index', function(Y, NAME) {

/**
 * The container-binder-index module.
 *
 * @module container-binder-index
 */

    /**
     * Constructor for the Binder class.
     *
     * @param mojitProxy {Object} The proxy to allow the binder to interact
     *        with its owning mojit.
     *
     * @class Binder
     * @constructor
     */
    Y.namespace('mojito.binders')[NAME] = {

        /**
         * Binder initialization method, invoked after all binders on the page
         * have been constructed.
         */
        init: function(mp) {
            mp.listen('tweet-highlighted', function(evt) {
                mp.broadcast('show-tweets', {
                    screenName: evt.data.name
                });
            });
        }

    };

}, '0.0.1', {requires: ['mojito-client']});
