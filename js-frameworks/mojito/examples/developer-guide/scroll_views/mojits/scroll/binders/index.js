/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint anon:true, sloppy:true, nomen:true*/

YUI.add('scroll-binder-index', function (Y, NAME) {

    Y.namespace('mojito.binders')[NAME] = {
        init: function (mojitProxy) {
            this.mojitProxy = mojitProxy;
        },
        bind: function (node) {
            var scrollView = new Y.ScrollView({
                id: 'scrollview',
                srcNode: node.one('#scrollview-content'),
                width: 320,
                flick: {
                    minDistance:10,
                    minVelocity:0.3,
                    axis: "x"
                }
            });
            scrollView.render();

            // Prevent default image drag behavior
            scrollView.get("contentBox").delegate("mousedown", function(e) {
                e.preventDefault();
            }, "img");
        }
    };
}, '0.0.1', {requires: ['scrollview']});
