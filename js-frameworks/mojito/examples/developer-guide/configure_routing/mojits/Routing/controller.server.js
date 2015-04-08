/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint anon:true, sloppy:true, nomen:true*/

YUI.add('routing', function(Y, NAME) {
    // Builds object containing route information
    function route_info(ac) {
        var methods = "",
            name = "",
            action = ac.action,
            path = ac.http.getRequest().url,
            routes = ac.config.getRoutes();
        if (action === "index" && path === "/") {
            name = "root_route";
            methods = 'GET';
        } else if (action==="index") {
            name = "index_route";
        } else {
            name = "show_route";
            methods = 'POST';
        }
        methods = methods.toUpperCase();
        return {
            "path": path,
            "name": name,
            "methods": methods.replace(/, $/, "")
        };
    }
    Y.namespace('mojito.controllers')[NAME] = {
        index: function (ac) {
            ac.done(route_info(ac));
        },
        show: function (ac) {
            ac.done(route_info(ac));
        }
    };
}, '0.0.1', {requires: ['mojito-config-addon', 'mojito-http-addon']});
