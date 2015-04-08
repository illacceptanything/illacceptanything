/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */


/*jslint anon:true, sloppy:true, nomen: true */
/*global YUI*/


/**
 * @module ActionContextAddon
 */
YUI.add('mojito-url-addon', function(Y, NAME) {


    /**
     * <strong>Access point:</strong> <em>ac.url.*</em>
     * Generates URL's based on the applictions routing configuration
     * @class Url.common
     */
    function UrlAcAddon(command, adapter, ac) {
        this._page = adapter.page;
        this.pathToRoot = this._page.staticAppConfig.pathToRoot;
        this.maker =  null;
    }


    UrlAcAddon.prototype = {

        namespace: 'url',
        /**
        Finds the first matching route from the given URL
        @method find
        @param {String} url the URL to find a route for.
        @param {String} the HTTP method. Default value is "get".
        **/
        find: function(url, verb) {

            // Remove http://some.domain.com/ stuff
            if (url.indexOf('http://') === 0) {
                url = url.slice(url.indexOf('/', 7));
            }

            // Remove an query params given
            if (url.indexOf('?') > 0) {
                url = url.slice(0, url.indexOf('?'));
            }

            return this.getRouteMaker().find(url, (verb || 'get').toLowerCase());
        },

        /**
        Given a name based on the route configuration, generate the URL.

        This is "similar" to the `make` API, except that this method uses the
        `name` of the route instead of the `call` of the mojit.

        @param {String} name the name of the route
        @param {Object|String} params optional
        @return {String} the requested URL based on the route configuration
        **/
        linkTo: function(name, params) {
            return this.getRouteMaker().linkTo(name, params);
        },

        /**
         * Generates a URL from the given parameters
         * @method make
         * @param {string} base Base mojit defined at the root level of the
         *     Mojito application configuration.
         * @param {string} action Action reference, concatenated to the base
         *     using a period (.) separator.
         * @param {object|string} routeParams used to resolve any parametrized
         *     path for the route. If string is provided, Y.QueryString.parse
         *     will be used.
         * @param {string} verb the HTTP method. Default value is "get".
         * @param {object|string} urlParams added to the looked up route as query
         *     params, this has priority over routeParams. If string is provided,
         *     Y.QueryString.parse will be used.
         */
        make: function(base, action, routeParams, verb, urlParams) {
            var url,
                key,
                params,
                query = base + '.' + action;

            params = (typeof routeParams === 'string' ? Y.QueryString.parse(routeParams) :
                    Y.merge(routeParams || {}));

            if (urlParams) {
                urlParams = (typeof urlParams === 'string' ? Y.QueryString.parse(urlParams) :
                        urlParams);

                // adding querystring params to routeParams and let
                // the url maker to create the proper url. Empty params
                // will be left out. TODO: why?
                for (key in urlParams) {
                    if (urlParams.hasOwnProperty(key) && urlParams[key]) {
                        params[key] = urlParams[key];
                    }
                }

            }

            url = this.getRouteMaker().make(query, (verb || 'get'), params);

            // IOS PATCH
            if (url && (typeof window !== 'undefined')) {
                url = Y.mojito.util.iOSUrl(url);
            }

            // this is mainly used by html5app
            if (url && this.pathToRoot) {
                url = this.pathToRoot + url;
            }

            return url;
        },

        getRouteMaker: function() {
            if (!this.maker) {
                this.maker = new Y.mojito.RouteMaker(
                    this._page.routes
                );
            }
            return this.maker;
        }

    };

    Y.namespace('mojito.addons.ac').url = UrlAcAddon;

}, '0.1.0', {requires: [
    'mojito-config-addon',
    'mojito-route-maker',
    'querystring-parse-simple',
    'mojito-util'
]});
