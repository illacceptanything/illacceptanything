/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint anon:true, sloppy:true, nomen:true*/

YUI.add('sender-binder-index', function (Y, NAME) {

    Y.namespace('mojito.binders')[NAME] = {

        init: function (mojitProxy) {
            this.mp = mojitProxy;
        },

        bind: function (node) {
            var mp = this.mp;
            this.node = node;
            // capture all events on "ul li a"
            this.node.all('ul li a').on('click', function(evt) {
                var url = evt.currentTarget.get('href');
                evt.halt();

                Y.log('Triggering fire-link event: ' + url, 'info', NAME);
                mp.broadcast('fire-link', {url: url});
            });
        }

    };

}, '0.0.1', {requires: ['node', 'mojito-client']});
