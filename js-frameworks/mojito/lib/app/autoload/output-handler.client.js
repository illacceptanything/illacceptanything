/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */


/*jslint anon:true, nomen:true, browser:true, plusplus:true*/
/*global YUI*/


YUI.add('mojito-output-handler', function(Y, NAME) {
    'use strict';

    var attachAssets,
        complete,
        loaded = {
            js: {},
            css: {}
        };


    // Attach assets found in the "assets" to the page
    attachAssets = function(assets, cb) {
        var toLoad = {
            css: [],
            js: []
        },
            done = {},
            blobNode,
            doneChecker,
            executeInlinedScripts;

        // Any scripts contain in assets.blob must be created using document.createElment('script'),
        // and then replaced, otherwise the scripts never get executed.
        executeInlinedScripts = function (node) {
            var i,
                child,
                script;

            if (node.tagName === 'SCRIPT') {
                script = document.createElement('script');
                script.text = node.text;
                for (i = 0; i < node.attributes.length; i++) {
                    if (node.attributes[i].specified) {
                        script[node.attributes[i].name] = node.attributes[i].value;
                    }
                }
                node.parentNode.replaceChild(script, node);
                return;
            }

            for (i = 0; i < node.children.length; i++) {
                child = node.children[i];
                executeInlinedScripts(child);
            }
        };

        Y.Object.each(assets, function(types, location) {
            Y.Object.each(types, function(list, type) {
                var i;

                if (type === 'blob') {
                    for (i = 0; i < list.length; i += 1) {
                        blobNode = Y.Node.create(list[i]);
                        if (blobNode) {
                            Y.one('head').append(blobNode);
                            // Ensure that inlined script get executed.
                            executeInlinedScripts(blobNode._node);
                        }
                    }
                } else {
                    for (i = 0; i < list.length; i += 1) {
                        if (!loaded[type][list[i]]) {
                            toLoad[type].push(list[i]);
                        }
                        loaded[type][list[i]] = true;
                    }
                }
            });
        });

        doneChecker = function(type) {
            if (type) {
                done[type] = true;
            }
            if (done.css && done.js) {
                cb();
            }
        };

        if (toLoad.css.length > 0) {
            // TODO: better error detection/handling.
            Y.Get.css(toLoad.css, {
                onEnd: function() {
                    doneChecker('css');
                }
            });
        } else {
            done.css = true;
        }
        if (toLoad.js.length > 0) {
            // TODO: better error detection/handling.
            Y.Get.script(toLoad.js, {
                onEnd: function() {
                    doneChecker('js');
                }
            });
        } else {
            done.js = true;
        }
        // in case we have neither (or either Y.Get calls return really fast)
        doneChecker();
    };


    /*
     * Handles final processing for done().
     * @method complete
     * @param {string} data The data to pass to the callback. Usually markup or
     *     JSON.
     * @param {Object} meta The meta object from the dispatch() call.
     * @param {MojitoClient} client The client instance.
     * @param {string} viewId An optional view ID for the mojit.
     * @param {Function} callback The callback function to invoke.
     */
    complete = function(data, meta, client, viewId, callback) {
        // If we get some JSON decode it
        if (meta && meta.http && meta.http.headers['content-type'] &&
                meta.http.headers['content-type'][0].indexOf(
                    'application/json'
                ) === 0) {
            data = Y.JSON.parse(data);
        }

        callback(null, data, meta);

        if (meta && meta.binders) {
            // DOM needs to render and return to main event loop before
            // attaching.
            window.setTimeout(function() {
                client.attachBinders(meta.binders, viewId, meta.view.id);
            });
        }
    };


    /*
     * This is an object used as the single pathway for data to leave a mojit
     * action execution. It is used as a component of the ActionContext object,
     * which uses it to call <em>done</em> and <em>flush</em> in order to
     * complete.
     *
     * There are two versions of this object, one for the client, and one for
     * the server. This is the client version, which is much simpler than the
     * server version.
     *
     * @class OutputHandler
     * @constructor
     * @param {String} viewId The view id of the current mojit binder
     *     responsible for this action execution
     * @param {Function} cb
     * @param {Object} mojitoClient
     */
    function OutputHandler(viewId, cb, mojitoClient) {
        this.viewId = viewId;
        this.callback = cb;
        this.buffer = '';
        this.mojitoClient = mojitoClient;
        this.page = mojitoClient.page;
    }


    OutputHandler.prototype = {

        flush: function(data, meta) {
            this.done(data, meta);
        },

        done: function(data, meta) {
            var client = this.mojitoClient,
                viewId = this.viewId,
                callback = this.callback;

            // Add meta to the page before going on
            if (meta && meta.assets) {
                attachAssets(meta.assets, function() {
                    complete(data, meta, client, viewId, callback);
                });
            } else {
                complete(data, meta, client, viewId, callback);
            }

        },

        error: function(err) {
            this.callback(err);
        }
    };

    Y.namespace('mojito').OutputHandler = OutputHandler;

}, '0.1.0', {requires: [
    'mojito',
    'json-parse',
    'node'
]});
