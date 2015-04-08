/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint anon:true*/
/*global YUI*/

YUI.add('read', function(Y, NAME) {
    'use strict';

    /**
     * Choose text display size.
     * @method size
     * @private
     * @param {Number} tlen Story title character count.
     * @param {Number} dlen Story description character count.
     * @return {String} Predefined css class.
     */
    function size(tlen, dlen) {
        var weighted = (tlen * 1.4) + dlen;
        return ((weighted > 850) && 'medium') ||
            ((weighted > 500) && 'large') ||
            ((weighted > 300) && 'x-large') || 'xx-large';
    }

    /**
     * Compose the data for the view.
     * @method compose
     * @private
     * @param {Object} feedmeta Feed metadata.
     * @param {Array.<Object>} stories The list of stories.
     * @return {Object} Data for view renderer (mustache.js).
     */
    function compose(feedmeta, stories) {
        var vu = {
                feedname: feedmeta.name,
                stories: stories || [],
                navdots: []
            },
            n = Math.max(0, vu.stories.length);

        Y.each(stories, function(story, i) {
            var curr = feedmeta.start + i,
                prev = curr - 1 < 1 ? n : curr - 1,
                next = curr + 1 > n ? 1 : curr + 1;

            if (story.title && story.description && story.link) {
                story.prev = '&start=' + prev;
                story.next = '&start=' + next;
                story.css_style = size(story.title.length, story.description.length);
                vu.navdots.push({});

            } else {
                Y.log('story ' + i + ' is missing data', 'warn');
            }
        });

        return vu;
    }

    /**
     * Something went wrong, render something.
     * @method fail
     * @private
     * @param {String} error The error message.
     * @param {ActionContext} ac The action context.
     */
    function fail(error, ac) {
        ac.done({title: 'oh noes!', stories: [{description: error}]});
    }

    /**
     * Load feed metadata for feed named in url, get data, display.
     * @method index
     * @param {ActionContext} ac The action context to operate on.
     */
    function index(ac) {
        var id = ac.params.merged('id'),
            conf = ac.config.get(),
            feeds = ac.config.getDefinition('feeds')[id],
            model = ac.models.get('rss'),
            error;

        if (feeds) {
            // Add metadata to feeds object before we pass it on
            feeds.id = id;
            feeds.start = +(ac.params.url('start')) || 1;
            feeds.limit = conf.limit;

        } else {
            error = 'configs for feed not found';
        }

        // Process feed data and call done.
        function afterQuery(error, feeds, response) {
            var vu = compose(feeds, response);
            vu.spaceid = conf.spaceid;

            return error ? fail(error, ac) : ac.done(vu);
        }

        // Ask model for feed data, or display error.
        return error ? fail(error, ac) : model.get(feeds, afterQuery);
    }

    /**
     * Display feed data in a horizontally flickable scrollview.
     * @class ReadController
     */
    Y.namespace('mojito.controllers')[NAME] = {
        index: index,
        test: {
            size: size,
            compose: compose
        }
    };

}, '0.0.1', {requires: [
    'mojito',
    'mojito-config-addon',
    'mojito-models-addon',
    'mojito-params-addon',
    'read-model-rss'
]});
