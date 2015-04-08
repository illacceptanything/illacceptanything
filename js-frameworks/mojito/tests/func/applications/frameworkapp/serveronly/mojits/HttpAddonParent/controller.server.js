/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('HttpAddonParent', function(Y, NAME) {

/**
 * The HttpAddonParent module.
 *
 * @module HttpAddonParent
 */

    /**
     * Constructor for the Controller class.
     *
     * @class Controller
     * @constructor
     */
    Y.namespace('mojito.controllers')[NAME] = {

        /**
         * Method corresponding to the 'index' action.
         *
         * @param ac {Object} The action context that provides access
         *        to the Mojito API.
         */
        index: function(ac) {
            //ac.http.addHeader('x-generated-by', 'Mojito');
            ac.http.addHeader('set-cookie', 'UserID=abc; Max-Age=3600');
            var header = ac.http.getHeader('set-cookie'); //('x-generated-by');

            // done:
            var template,
                cfg = ac.mojit.config,
                children = cfg.children;

            template = {};

//            ac.dispatch(cfg.children[0], {done: function() {}, flush: function() {}});

            ac.composite.execute(cfg, function(data, meta) {
                console.log("CHILD meta:");
                console.log(meta.http.headers);
                var merged = Y.merge(template, data);
                ac.done(merged, meta);

            }, this);

        },

        testRequestObj: function(ac){
            var reqObj = ac.http.getRequest();
            //console.log(reqObj);
            ac.http.setHeader('content-type', 'text/html');
            ac.done({
                method: reqObj.method,
                url: reqObj.url,
                trailers: reqObj.trailers,
                httpVersion: reqObj.httpVersion,
                headers: JSON.stringify(reqObj.headers)
            });
        },

        testSimpleRedirect: function(ac){
            var testMethod = ac.params.getFromUrl('method');
            var mojitName = ac.params.getFromUrl('mojit');
            var action = ac.params.getFromUrl('action');
            //ac.http.redirect(mojitName, action, {}, testMethod);
            ac.http.redirect('/' + mojitName + "/" + action);
        },

        callWSWithXhr: function(ac){
            var reqObj = ac.http.getRequest();
            var hostPort = reqObj.headers.host;
            var isXhr = ac.params.getFromUrl('isXhr') || 'false';
            ac.models.get('model').callWS(hostPort, isXhr, function(error, response){
                if (!error)
                {
                    ac.http.setHeader('content-type', 'text/html');
                    ac.done(response.getBody());
                }
                else
                {
                    ac.done(error.responseText);
                }
            });
        },

        checkingXhr: function(ac) {
            var isXhr = ac.http.isXhr();
            ac.done("<p id=\"xhrValue\">This is the Xhr value: " + isXhr + "</p>");
        },

        testResponseObj: function(ac){
            var response = ac.http.getResponse();

            var data = {
                headers : JSON.stringify(response._headers),
                shouldKeepAlive : response.shouldKeepAlive,
                hasBody : response._hasBody
            };
            ac.http.setHeader('content-type', 'text/html');
            ac.done(data);
        },

        testAddSetHeader: function(ac){
            ac.http.addHeader('my_header', 'my_header1_value');
            var option = ac.params.getFromUrl('header_option') || "add";
            if (option == 'add')
            {
                console.log("I am adding the header");
                ac.http.addHeader('my_header', 'my_header2_value');
            }
            else if (option == "set")
            {
                ac.http.setHeader('my_header', 'my_final_header_value');
            }
            ac.http.setHeader('content-type', 'text/html');
            ac.done("<p>I am done...Please check for the headers.</p>");
        },

        testAddSetHeaders: function(ac){
            var headers = {
                    "foo": "bar",
                    "foo1": "bar1",
                    "foo2": "bar2"
            };
            ac.http.addHeaders(headers);
            var option = ac.params.getFromUrl('header_option') || "add";
            if (option == 'add')
            {
                ac.http.addHeaders({
                    "foo": "bar_one_more",
                    "foo1": "bar1_one_more"
                });
            }
            else if (option == "set")
            {
                ac.http.setHeaders({
                    "foo": "bar_final",
                    "foo1": "bar1_final",
                    "foo2": "" //this will remove the header
                });
            }
            ac.http.setHeader('content-type', 'text/html');
            ac.done("I am done...Please check for the headers.");
        },

        testGetHeaders: function(ac){
            var allHeaders = ac.http.getHeaders();
            var valueMatch = "true";

            Y.Object.each(allHeaders, function(headerValue, headerKey) {
                var valueFromCommand = ac.http.getHeader(headerKey);
                if (valueFromCommand == headerValue)
                {
                    valueMatch = valueMatch + "true";
                }
                else
                {
                    valueMatch = valueMatch + "false";
                }
            });

            ac.http.setHeader('content-type', 'text/html');
            if (valueMatch.indexOf("false") == -1)
            {
                ac.done("<p id=\"output\">All Headers match</p>");
            }
            else
            {
                ac.done("<p id=\"output\">All Headers DOES NOT match</p>");
            }
        },

        testHeadersWithChild: function(ac){
            var template,
                cfg = ac.mojit.config,
                children = cfg.children;

            template = {};
            ac.composite.execute(cfg, function(data, meta) {
                console.log("CHILD meta:");
                console.log(meta);
                console.log(meta.http.headers);
                ac.http.setHeader('my_header', 'ByParent');
                var merged = Y.merge(template, data);
                ac.done(merged, meta);
            }, this);
        }
    };

}, '0.0.1', {requires: [
    'mojito',
    'mojito-http-addon',
    'mojito-params-addon',
    'mojito-models-addon',
    'mojito-composite-addon',
    'HttpAddonParentModel']});

