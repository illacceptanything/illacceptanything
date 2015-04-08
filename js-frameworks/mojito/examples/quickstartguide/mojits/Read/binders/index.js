/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/**
Binder for Mojito

Each mojit you create can have some specific code called binders that is only deployed to the browser. 
The code can perform the following three functions:
    - allow event handlers to attach to the mojit DOM node
    - communicate with other mojits on the page
    - execute actions on the mojit that the binder is attached to

For more info, visit: http://developer.yahoo.com/cocktails/mojito/docs/intro/mojito_binders.html
**/

/*jslint anon:true, sloppy:true*/
/*global YUI*/


YUI.add('read-binder-index', function(Y, NAME) {

    Y.namespace('mojito.binders')[NAME] = {

        init: function(mojitProxy) {
            this.mp = mojitProxy;
            this.spaceid = this.mp.config.spaceId;
        },

        /**
        Setup client-side event handlers.
        @param {Node} mojitNode The node for the mojit's outer div.
        **/
        bind: function(mojitNode) {

            // Open all links in the guide in new tabs.
            mojitNode.all('.content #desc a').on('click', function(evt) {
                evt.preventDefault();
                window.open(evt.target.get("href"), '_blank');
            });

        }
    };

}, '0.0.1', {requires: [
    'anim',
    'node-event-delegate',
    'scrollview',
    'scrollview-paginator'
]});
