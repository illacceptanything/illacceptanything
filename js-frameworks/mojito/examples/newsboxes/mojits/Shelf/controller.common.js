/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*global YUI*/


YUI.add('shelf', function (Y, NAME) {
    'use strict';

    /**
     * Display a clickable tile for each feed in ./definition.json.
     * N.B. static html5app build can't use routes, so use .html.
     * @param {ActionContext} ac The action context.
     */
    function index(ac) {
        var vudata = { // Mustache template data.
                tiles: []
            };

        Y.each(ac.config.getDefinition('feeds'), function (feed, id) {
            feed.link = 'read.html?id=' + encodeURIComponent(id);
            vudata.tiles.push(feed);
        });

        ac.composite.done(vudata, {
            view: {
                name: 'index'
            }
        });
    }

    /**
     * Display feed titles in a grid of scrollable tiles. Feed data from
     * ./definition.json.
     * @class ShelfController
     */
    Y.namespace('mojito.controllers')[NAME] = {
        index: index
    };

}, '0.0.1', {requires: [
    'mojito',
    'mojito-config-addon',
    'mojito-composite-addon'
]});
