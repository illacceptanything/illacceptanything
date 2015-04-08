/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint anon:true, sloppy:true, nomen:true*/

YUI.add('receiver', function (Y, NAME) {

    Y.namespace('mojito.controllers')[NAME] = {
        index: function (actionContext) {
            actionContext.done({title: 'This is the receiver mojit'});
        },
        show: function (actionContext) {
            var url = actionContext.params.merged('url') || "http://farm1.static.flickr.com/21/35282840_8155ba1a22_o.jpg";
            actionContext.done(
                {
                    title: 'Image matching the link clicked on the left.',
                    url: url
                },
                { view: { binder: 'index' } }
            );
        }
    };

}, '0.0.1', {requires: ['mojito', 'mojito-params-addon']});
