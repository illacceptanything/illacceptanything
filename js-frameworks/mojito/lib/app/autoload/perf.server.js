/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */
/*jslint anon:true, node: true, nomen:true*/
/*global YUI,require*/
YUI.add('mojito-perf', function (Y, NAME) {

    'use strict';

    /**
     * @module mojito-perf
     * @class MojitoPerf
     * @static
     */

    var libfs = require('fs'),

        buffer = {},
        config = Y.config.perf,

        requestId  = 0,
        colorRed   = '\u001b[31m',
        colorReset = '\u001b[0m',

        getgo,
        microtime;


    try {
        microtime = require('microtime');
    } catch (e) {
        Y.log('microtime not found. Recorded times will not have' +
            ' microsecond accuracy', 'warn', NAME);
    }


    //internal. write perf info into a file
    function writeLog(filename, logs) {
        var outstream,
            i;

        try {
            outstream = libfs.createWriteStream(filename.replace('{req}', requestId), {
                flags: 'a' // append
            });
            for (i = 0; i < logs.length; i += 1) {
                outstream.write(logs[i].join('|') + "\n");
            }
            outstream.end();
            outstream = null;
        } catch (err) {
            Y.log('Error trying to dump perf metrics in file: ' +
                filename + ' Error:' + err, 'error', NAME);
        }
    }


    //internal. print perf info in the logs
    function print(group, key) {
        var o = buffer[group][key],
            type = (o.ms ? 'TIMELINE' : 'MARK'),
            // if we already have milliseconds, good
            // if not, we can compute it based on request init
            time = o.time,
            offset = o.time - getgo,
            duration = o.ms || '',
            desc = o.msg || 'no description',
            label = o.label,
            id = o.id;

        if ((config.mark && !o.ms) || (config.timeline && o.ms)) {

            Y.log(group + ':' + key + ' ' + type + colorReset +
                ' offset=' + colorRed + offset + colorReset +
                (o.ms ? ' duration=' + colorRed + duration + colorReset : '') +
                ' (' + desc + ')',
                'mojito', NAME);

            return [type, requestId, time, duration, group, label, id, desc];

        }

    }


    //internal. abstracts where timestamps come from
    function timestamp() {
        return microtime ? microtime.now() : new Date().getTime();
    }


    /**
     * Produces an ID to identify the timeline or mark based on a
     * command object.
     *
     * @method idFromCommand
     * @private
     * @param {object} command Object that represent the command to invoke.
     * @return {string} ID that represents the command.
     **/
    function idFromCommand(command) {
        var str;
        if (command && command.instance) {
            if (command.instance.id) {
                str = command.instance.id;
            } else if (command.instance.base) {
                str = '+' + command.instance.base;
            } else {
                str = '@' + command.instance.type;
            }
            str += '.' + (command.action || command.instance.action || '???');
        }
        return str;
    }


    /**
     * Sets a mark in the request timeline. All marks will be flushed
     * after the end. This is useful to measure when a particular process
     * start or end with respect to the request timeline.
     *
     * @method mark
     * @param {string} group Event group.
     * @param {string} label Event identifier. Will be combined with group.
     * @param {string} msg Description of the mark.
     * @param {string|object} id Unique identifier of the mark, usually
     *      the requestId or a command object.
     * @return {Object} The mark entry.
     **/
    function mark(group, label, msg, id) {
        var s,
            key = label;

        if (!group || !label) {
            return;
        }

        if (id) {
            // we might also accept a command object
            id = Y.Lang.isObject(id) ? idFromCommand(id) : id;
            key += '[' + id + ']';
        }

        if (!buffer[group]) {
            buffer[group] = {};
        }

        if (!msg) {
            msg = '';
        }

        if (buffer[group][key]) {
            Y.log('Perf metric collision for group=' + group +
                ' label=' + label + ' id=' + id +
                '. Measure one thing at a time.', 'warn', NAME);
            key += Y.guid();
        }

        s = buffer[group][key] = {};
        s.msg = msg;
        s.label = label;
        s.id = id;
        s.time = timestamp();
        return s;
    }


    /**
     * Starts a timeline metric, providing a way to call it done
     * at some point in the future. This is useful to measure the
     * time to execute a process in mojito.
     *
     * @method timeline
     * @param {string} group Event group.
     * @param {string} label Event identifier. Will be combined with group.
     * @param {string} msg Description of the mark.
     * @param {string} id Unique identifier of the mark, usually
     *      the requestId or the yuid().
     * @return {object} represents the timeline object that has a method
     *      called "done" that can be invoked when the process finish.
     **/
    function timeline(group, label, msg, id) {
        var m = mark(group, label, msg, id);
        return {
            done: function () {
                m.ms = timestamp() - m.time;
            }
        };
    }


    /**
     * Dumps all marks and timeline entries into the console.
     * This method is meant to be called automatically when
     * a request ends. You can target specific metrics by using
     * the configuration:
     *
     * "perf": {
     *    "include": {
     *        "mojito-action-context": true
     *    }
     * }
     *
     * Or just exclude some of them by doing:
     *
     * "perf": {
     *    "exclude": {
     *        "mojito-action-context": true
     *    }
     * }
     *
     *
     * @method dump
     * @private
     * @return {array} collection of perf logs. Each item will expose:
     *     {type, requestId, time, duration, group, label, id, desc}
     **/
    function dump() {

        var group,
            key,
            entry,
            logs = [];

        for (group in buffer) {
            if ((buffer.hasOwnProperty(group)) &&
                    (!config.exclude || !config.exclude[group]) &&
                    (!config.include || config.include[group])) {
                for (key in buffer[group]) {
                    if (buffer[group].hasOwnProperty(key)) {
                        entry = print(group, key);
                        if (entry) {
                            logs.push(entry);
                        }
                    }
                }
            }
        }
        buffer = {};
        // dumping to disk
        if (config.logFile) {
            Y.log('Dumping performance metrics into disk: ' +
                config.logFile, 'mojito', NAME);
            writeLog(config.logFile, logs);
        }

        return logs;
    }


    /**
     * Instruments requests that will be processed by mojito
     * core, providing a valid timeline for that request, and
     * allowing to instrument some other relative processes,
     * and grouping them per request to facilitate analysis.
     * This method is responsible for calling "dump".
     *
     * @method instrumentMojitoRequest
     * @param {object} req the request object from express.
     * @param {object} res the response object from express.
     **/
    function instrumentMojitoRequest(req, res) {
        var id = (requestId += 1),
            perf,
            end = res.end;

        getgo = timestamp();
        if (Y.Object.keys(buffer).length > 0) {
            Y.log('Multiple requests at the same time. This can ' +
                    'mess with the perf analysis. Curl is your best ' +
                    'friend, use it.', 'warn', NAME);
        }

        perf = timeline('mojito', 'request', 'the whole request', id);

        // hooking into the res.end called from output-handler.server.js
        // to be able to flush perf metrics only for mojito requests.
        // static requests and other type of requests will be ignored.
        res.end = function () {
            if (perf) {
                end.apply(res, arguments);
                Y.log('Flushing perf metrics', 'mojito', NAME);
                perf.done();
                dump();
                // some cleanup
                perf = null;
                end = null;
                req = null;
                res = null;
            }
        };
    }

    if (config) {
        Y.namespace('mojito.perf');
        Y.mojito.perf.idFromCommand = idFromCommand;
        Y.mojito.perf.instrumentMojitoRequest = instrumentMojitoRequest;
        Y.mojito.perf.dump = dump;

        // overriding the default definitions if needed
        if (config.timeline) {
            Y.mojito.perf.timeline = timeline;
        }
        if (config.mark) {
            Y.mojito.perf.mark = mark;
        }
    } else {
        config = {};
    }

    // Hook profiles in
    Y.mojito.hooks.registerHook(NAME, 'adapterBuffer', function(w, adapter) {
        if (w === 'start') {
            this.ab_perf = Y.mojito.perf.timeline('mojito-composite-addon', 'child', 'the whole child', adapter.id);
        } else {
            this.ab_perf.done();
        }
    });
    Y.mojito.hooks.registerHook(NAME, 'addon', function(w, addOn, cfg) {
        if (w === 'start') {
            this.ad_perf = Y.mojito.perf.timeline('mojito-composite-addon', 'execute', Y.Object.keys(cfg.children).join(','), addOn.ac.command);
        } else {
            this.ad_perf.done();
        }
    });
    Y.mojito.hooks.registerHook(NAME, 'hb', function(w, tmpl) {
        if (w === 'start') {
            this.hb_perf = Y.mojito.perf.timeline('mojito', 'hb:render', 'time to render a template', tmpl);
        } else {
            this.hb_perf.done();
        }
    });
    Y.mojito.hooks.registerHook(NAME, 'attachActionContext', function(w, command) {
        if (w === 'start') {
            this.acc_perf = Y.mojito.perf.timeline('mojito', 'ac:addons', 'attaching addons to AC object', command);
        } else {
            this.acc_perf.done();
        }
    });
    Y.mojito.hooks.registerHook(NAME, 'actionContext', function(w, ac, opts) {
        if (w === 'start') {
            this.ac_perf = Y.mojito.perf.timeline('mojito', 'ac:init', 'set up AC object', opts.command);
        } else if (w === 'end1') {
            this.ac_perf.done();
            Y.mojito.perf.mark('mojito', 'action:start', 'before the action', opts.command);
            this.ac_perf = Y.mojito.perf.timeline('mojito', 'action:call', 'the initial syncronous part of the action', opts.command);
        } else {
            this.ac_perf.done();
        }
    });
    Y.mojito.hooks.registerHook(NAME, 'actionContextDone', function(w, ac) {
        if (w === 'start') {
            this.acd_perf = Y.mojito.perf.timeline('mojito', 'ac.done', 'time to execute ac.done process', ac.command);
        } else if (w === 'end1') {
            this.acd_perf.done();
        } else {
            this.acd_perf.done();
            Y.mojito.perf.mark('mojito', 'action:stop', 'after the action', ac.command);
        }
    });
    Y.mojito.hooks.registerHook(NAME, 'dispatchCreateAction', function(w, command) {
        if (w === 'start') {
            this.dac_perf = Y.mojito.perf.timeline('mojito', 'ac:ctor', 'create ControllerContext', command);
        } else {
            this.dac_perf.done();
        }
    });
    Y.mojito.hooks.registerHook(NAME, 'dispatch', function(w, command) {
        if (w === 'start') {
            this.dis_perf = Y.mojito.perf.timeline('mojito', 'dispatch:expandInstance', 'gather details about mojit', command);
        } else {
            this.dis_perf.done();
        }
    });
    Y.mojito.hooks.registerHook(NAME, 'AppDispatch', function(req, res) {
        // if perf metrics are on, we should hook into
        // the mojito request to flush metrics when
        // the connection is closed.
        if (Y.mojito.perf.instrumentMojitoRequest) {
            Y.mojito.perf.instrumentMojitoRequest(req, res);
        }
    });

}, '0.1.0', {requires: [
    'mojito',
    'mojito-hooks'
]});
