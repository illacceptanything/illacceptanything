/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*global YUI*/


YUI.add('read-model-rss', function (Y, NAME) {
    'use strict';

    /**
     * Pass video url to player.
     * @param {Object} enclosure An object of the form:
     *     {length:"123", type:"video/mp4", url:".."} | undefined.
     * @param {Number} num int story number, starting from 0.
     * @return {String} An appropriate HTML chunk for the URL.
     */
    function getLink(enclosure, num) {
        var url = enclosure && enclosure.url,
            pad = new Array(900).join(' '); // For small headline css size.

        return url ?
                '<div id="videobox' + num + '">' + url + '</div>' + pad :
                '';
    }

    /**
     * Strip HTML tags from the content string provided.
     * @param {String} content The content, possibly containing markup.
     * @return {String} Plain text.
     */
    function stripTags(content) {
        return Y.Lang.trim(content.replace(/<\/?\w+[\s\S]*?>/gmi, ' '));
    }

    /**
     * Handle result data processing.
     * @param {Object} YQL response, i.e.
     *     http://query.yahooapis.com/v1/public/yql
     *         ?q=SELECT+title,link,pubDate,description,
     *         enclosure+FROM+rss+WHERE+url=
     *         "http://feeds.feedburner.com/TechCrunch"
     *         &format=json.
     * @return {Array.<Object>} The 'rows' of result data.
     */
    function processResponse(response, limit) {
        var stories,
            i,
            story,
            list = [],
            error = null;

        stories = (response.query &&
            response.query.results &&
            response.query.results.item) || [];

        for (i in stories) {
            if (stories.hasOwnProperty(i)) {

                story = {
                    title: Y.Lang.trim(stories[i].title),
                    link: Y.Lang.trim(stories[i].link),
                    pubDate: +stories[i].pubdate || +new Date(),
                    description: getLink(stories[i].enclosure, i) ||
                        stripTags(stories[i].description)
                };

                if (story.title && story.description && story.link) {
                    list.push(story);
                    if (list.length >= limit) {
                        break;
                    }
                } else {
                    Y.log('skipping story ' + i + ': missing data', 'warn');
                }
            }
        }

        return list;
    }

    /**
     * Handle error responses.
     * @param {Object} response The YQL response.
     * @return {String} HTTP status message if any.
     */
    function processError(response) {
        return response.query &&
            response.query.diagnostics &&
            response.query.diagnostics.url &&
            response.query.diagnostics.url['http-status-message'];
    }

    /**
     * Fetch YQL RSS response as normalized json.
     * @param {Object} feedmeta Metadata for the selected feed.
     * @param {Function} callback The callback function to invoke.
     */
    function get(feedmeta, callback) {
        var query =
                'SELECT title,link,pubDate,description,enclosure ' +
                'FROM feed WHERE url=@feedurl ' +
                'LIMIT @feedlimit OFFSET @feedstart',
            param = {
                feedurl: feedmeta.url,
                feedlimit: feedmeta.limit,
                feedstart: feedmeta.start,
                format: 'json'
            };

        function afterYql(response) {
            var list = processResponse(response, feedmeta.limit) || [],
                error = null;

            // Error?
            if (!list.length) {
                error = 'Ooo, could not fetch stories for ' + feedmeta.name;
            }

            // Pass feedmeta through.
            callback(error, feedmeta, list);
        }

        Y.YQL(query, afterYql, param);
    }

    /**
     * Fetch normalized RSS feed data as JSON via YQL.
     * @class ReadModelRss
     */
    Y.mojito.models[NAME] = {
        get: get,
        test: {
            processResponse: processResponse,
            processError: processError,
            stripTags: stripTags
        }
    };

}, '0.0.1', {requires: [
    'mojito',
    'yql',
    'jsonp-url'
]});
