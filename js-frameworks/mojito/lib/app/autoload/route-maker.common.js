/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */


/*jslint anon:true, sloppy:true, regexp: true, nomen:true*/
/*global YUI,console*/

YUI.add('mojito-route-maker', function (Y, NAME) {

    var CACHE = { routes: {} };

    /**
    Given a route with parametrized variable, this function will attempt to
    resolve it based on a set of parameters.

    @method resolveParams
    @private
    @param {Object} route
    @param {Object} params
    @return {Object|null} the route if there is a match, otherwise null
    **/
    function resolveParams(route, params) {
        var keys = route.keys,
            len = keys.length,
            i;

        if (len === 0) {
            return route;
        }

        for (i = 0; i < len; i = i + 1) {
            if (!params[keys[i].name]) {
                return null;
            }
        }

        return route;
    }



    /**
    @class Maker
    @namespace Y.mojito
    @param {Object} routes routes configuration 
    @param {Boolean} init if true, reset the routes cache
    **/
    function Maker(routes, init) {

        if (init) {
            CACHE.routes = {};
        }

        CACHE.routes = routes;
    }

    Maker.prototype = {
        // For unit tests only
        // DO NOT USE
        _resolveParams: resolveParams,
        //

        /**
        Finds a route for a given method+URL
        @method find
        @param {String} url the URL to find a route for.
        @param {String} verb the HTTP method.
        @return {Object} the matched route object, or null
        **/
        find: function(uri, verb) {
            var routes = CACHE.routes,
                copy,
                i,
                j,
                key,
                len,
                matches,
                name,
                names,
                regex,
                route;

            for (name in routes) {
                if (routes.hasOwnProperty(name)) {
                    matches = routes[name].regexp.exec(uri);
                    if (matches) {
                        route = routes[name];
                        break;
                    }
                }
            }

            if (!route) {
                return null;
            }

            // Return clone to caller
            copy = Y.mojito.util.copy(route);

            // Extract the url paramterized paths and store in `params`
            copy.params = copy.params || {};
            len = copy.keys.length;
            for (i = 0, j = 1; i < len; i += 1, j += 1) {
                key = copy.keys[i];
                copy.params[key.name] = matches[j];
            }

            // Lookup the `call` by aliases
            names = route.annotations.aliases;
            key = verb + '#';
            name = undefined;
            for (i = 0; i < names.length; i += 1) {
                if (names[i].indexOf(key) > -1) {
                    name = names[i];
                    break;
                }
            }
            if (!name) {
                // The route is misconfigured, return error;
                // Most likely app.map() was not called with the
                // correct alias for the route
                return null;
            }
            copy.call = name.substring(key.length);

            // Do "call" replacement
            len = copy.keys.length;
            for (i = 0; i < len; i += 1) {
                key = copy.keys[i].name;
                regex = new RegExp('{' + key + '}', 'g');
                if (regex.test(copy.call)) {
                    copy.call = copy.call.replace(regex, copy.params[key]);
                }
            }

            return copy;
        },

        /**
        TODO How can we leverage express-map#pathTo ?

        Returns the matching route path based on the route name.

        @method linkTo
        @param {String} name
        @param {Object} params
        @return {String|null} path
        **/
        linkTo: function(name, params) {
            var route = CACHE.routes[name],
                path,
                keys,
                key,
                i,
                len,
                param,
                regex;

            if (!route) {
                return null;
            }

            path = route.path;
            keys = route.keys;

            len = keys.length;
            if (params && len > 0) {
                for (i = 0; i < len; i += 1) {
                    key = keys[i];
                    param = key.name || key;
                    regex = new RegExp('[:*]' + param + '\\b');
                    path = path.replace(regex, params[param]);
                }
            }

            // Replace missing params with empty strings.
            return path.replace(/([:*])([\w\-]+)?/g, '');
        },

        /**
        Generates a URL from a route query

        NOTE: Adding query string to "query" (e.g. "foo.index?src=dot") is
              deprecated feature.

        @method make
        @param {String} query string to convert to a URL
        @param {String} verb http method
        @param {Object} params object representing extra querystring
                               params. `query` might have querystring portion
                               portion, in which case they have priority.
        @return {String} the generated path uri for the query, or null
        **/
        make: function(query, verb, params) {
            var parts = query.split('?'),
                call = parts[0],
                residual = {},
                i,
                k,
                key,
                keys,
                paramKeys,
                route,
                uri;

            query = query || '';
            verb = verb || 'get';
            params = params || {};

            if (parts[1]) {
                params = Y.QueryString.parse(parts[1]);
            }

            key = Y.mojito.util.encodeRouteName(verb, call);
            // Y.log('Lookup key: ' + key, 'debug', NAME);
            route = CACHE.routes[key];

            if (!route) {
                return null;
            }

            // Check that the params is applicable to the route
            route = resolveParams(route, params);
            if (!route) {
                return null;
            }

            uri = route.path;

            keys = [];
            Y.Array.each(route.keys, function (key) {
                keys.push(key.name);
            });
            paramKeys = Y.Object.keys(params).sort();
            // Y.log('---- paramKeys: ' + paramKeys, 'debug', NAME);

            // handle left over params by appending them to query string
            for (i = paramKeys.length - 1; i >= 0; i = i - 1) {
                k = paramKeys[i];
                if (params.hasOwnProperty(k)) {
                    if (keys.indexOf(k) > -1) {
                        uri = uri.replace(':' + k, params[k]);
                        // Y.log('---- replacing key : ' + k + '; uri = ' + uri,
                        //       'debug', NAME);
                    } else {
                        residual[k] = params[k];
                    }
                }
            }

            if (!Y.Object.isEmpty(residual)) {
                uri += '?' + Y.QueryString.stringify(residual);
            }

            return uri;
        },

        /**
         * For optimization. Call this to get the computed routes that can be
         * passed to the constructor to avoid recomputing the routes.
         *
         * @method getComputedRoutes
         * @return {object} computed routes.
         */
        getComputedRoutes: function() {
            // NOTE: We used to copy() here. Research suggested that it was
            // safe to drop.

            return CACHE.routes;
        }
    };

    Y.namespace('mojito').RouteMaker = Maker;

}, '0.1.0', {  requires: [
    'querystring-stringify-simple',
    'querystring-parse',
    'mojito-util'
]});

