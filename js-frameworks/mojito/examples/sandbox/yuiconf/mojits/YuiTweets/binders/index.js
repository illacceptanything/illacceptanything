/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add('YuiTweetsBinderIndex', function(Y, NAME) {

    Y.namespace('mojito.binders')[NAME] = {

        init: function(mojitProxy) {
            this.mojitProxy = mojitProxy;
        },

        bind: function(node) {
            var mp = this.mojitProxy,
                tweetDivs;
            this.node = node;
            tweetDivs = node.all('.yui-tweet');

            tweetDivs.on('mouseover', function(evt) {
                var tweet = evt.currentTarget,
                    name = tweet.one('.screenName').getContent();
                tweet.addClass('highlighted');
                mp.broadcast('tweet-highlighted', {name: name});
            });
            tweetDivs.on('mouseout', function(evt) {
                evt.currentTarget.removeClass('highlighted');
            });
        }

    };

}, '0.0.1', {requires: ['mojito-client']});
