/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/**
Controller for Mojio

After an application has been configured to use a mojit, the mojit controller can either do all of the work or
delegate the work to models and/or views. In the typical case, the mojit controller requests the model to
retrieve data and then the controller serves that data to the views.
For more info, visit: http://developer.yahoo.com/cocktails/mojito/docs/intro/mojito_mvc.html#controllers
**/

/*jslint anon:true*/
/*global YUI*/

/**
Displays guide content in a horizontally flickable scrollview.
@class ReadController
**/
YUI.add('Read', function(Y, NAME) {
    'use strict';
    var LIB_MD;// Markdown library

    /**
    Is used for library initializations under test environment.

    @method init_test
    @private
    @param {Object} config The configuration object.
    **/
    function init_test(config) {
        // Mocking the library for test purposes.
        // Unit tests are run using "mojito test app app_root/".
        // Make sure the attributes are set up correctly, otherwise don't set at all
        if (config && config.test && config.test.libs) {
            LIB_MD = config.test.libs.lib_md;
        }
    }

    /**
    Composes the view with links to adjacent guides
    return callback({
                     title: guide title
                     content: guide content
                     prev: link to previous guide
                     next: link to next guide
                    })

    @method compose
    @private
    @param {Object} guidemeta Guide meta.
    @param {Object} result data of guide content.
    @param {Object} action context.
    @param (Function) callback for the final view data.
    **/
    function compose(guidemeta, data, ac, callback) {
        var vu = {},
            model = ac.models.get('guide'),
            afterGetAdjacentFiles = function (prevFilename, nextFilename) {
                // Checks filenames are not null
                if (prevFilename && nextFilename) {
                    vu.prev = ac.url.make('read', 'index', {
                        'filename': encodeURIComponent(prevFilename)
                    });
                    vu.next = ac.url.make('read', 'index', {
                        'filename': encodeURIComponent(nextFilename)
                    });
                }

                // Passes back view data
                callback(vu);
            };

        // Sets up title and content of view data
        vu.title = data.title;
        // Render Markdown content into HTML
        // Markdown requires node module installing. Please run "npm i" from project folder.
        vu.content = LIB_MD ? LIB_MD(data.content) : require("node-markdown").Markdown(data.content);

        // Gets filenames of all guides.
        model.getAdjacentGuideFilenames(guidemeta.filename, afterGetAdjacentFiles);
    }

    /**
    Something went wrong, RENDERS 'oh no' message.
    @method fail
    @private
    @param {String} error The error message.
    @param {ActionContext} ac The action context.
    **/
    function fail(error, ac) {
        ac.done({
            // Localize program generated text. See .js files under lang/
            title: ac.intl.lang("ERROR_TITLE"),
            content: error
        });
    }

    /**
    Loads guide's filename from URL, fetch the content from model and display it.
    @method index
    @param {ActionContext} ac The action context to operate on.
    **/
    function index(ac) {
        var guidemeta = {},
            model = ac.models.get('guide'),
            error,
            afterComposed = function(viewData) {
                ac.done(viewData);
            },
            afterGetGuide = function (error, resultObj) {
                // resultObj.title contains guide title
                // resultObj.content contains HTML content

                // Checks to see if there's an error
                if (error) {
                    fail(error, ac);
                } else {
                    // Normalize content
                    compose(guidemeta, resultObj, ac, afterComposed);
                }
            };

        // Fills in feed metas
        guidemeta = ac.params.merged();

        // Asks model for content, or display error.
        model.getGuide(guidemeta, afterGetGuide);
    }

    /**
    Register index method under controller.
    Keep all other private functions under "test" for unit testing.
    **/
    Y.namespace('mojito.controllers')[NAME] = {
        index: index,
        test: {
            init_test: init_test,
            compose: compose,
            fail: fail
        }
    };

}, '0.0.1', {requires: [
    'mojito',
    'mojito-config-addon',
    'mojito-models-addon',
    'mojito-params-addon',
    'mojito-url-addon',
    'mojito-intl-addon',
    'guide-model'
]});
