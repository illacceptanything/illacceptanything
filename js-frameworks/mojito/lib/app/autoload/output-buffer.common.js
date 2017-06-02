/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */


/*jslint anon:true, nomen:true*/
/*global YUI*/


/**
 * @module OutputBuffer
 * @requires mojito-util
 */
YUI.add('mojito-output-buffer', function (Y, NAME) {

    'use strict';

    /**
    OutputBuffer is an advanced adapter that can be use to dispatch a command
    while controlling the output of the operation for that command. This is
    useful for composite executions, and child rendering.

        var newAdapter = new Y.mojito.OutputBuffer('foo', function (err, data, meta) {
            // do something here...
        });
        newAdapter = Y.mix(newAdapter, originalAdapter); // inherit mojito stuff
        dispatcher(newCommand, newAdapter);

    The `newAdapter` exposes a regular adapter api, which means it has `flush`,
    `done` and `error` methods, plus anything else coming from `originalAdapter`.

    @class OutputBuffer
    @constructor
    @param {string} id identified for the mojit instance/action to be buffered
    @param {function} callback
    **/
    function OutputBuffer(id, callback) {
        this.id = id;
        this.data = '';
        this.meta = undefined;
        this.callback = function () {
            if (callback) {
                callback.apply(this, arguments);
            }
            callback = function () {
                Y.log('ac.done/flush/error called multiple times for: ' +
                        id, 'warn', NAME);
            };
        };
    }

    OutputBuffer.prototype = {

        done: function (data, meta) {
            this.data += data;
            // this trick is to call metaMerge only after the first pass
            this.meta = (this.meta ? Y.mojito.util.metaMerge(this.meta, meta) : meta);
            // calling back with the merged data and metas
            this.callback(null, this.data, this.meta);
        },

        flush: function (data, meta) {
            this.data += data;
            // this trick is to call metaMerge only after the first pass
            this.meta = (this.meta ? Y.mojito.util.metaMerge(this.meta, meta) : meta);
        },

        error: function (err) {
            Y.log("Error executing: '" + this.id + "':", 'error',
                    NAME);
            if (err.message) {
                Y.log(err.message, 'error', NAME);
            } else {
                Y.log(err, 'error', NAME);
            }
            if (err.stack) {
                Y.log(err.stack, 'error', NAME);
            }

            this.callback(err);
        }

    };

    Y.namespace('mojito').OutputBuffer = OutputBuffer;

}, '0.1.0', {requires: ['mojito-util']});
