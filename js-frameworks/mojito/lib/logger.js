/**
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */


/*jslint nomen:true, node:true*/

/**
Submodule used by mojito to setup logger related tasks.

@module mojito
@submodule logger
**/

'use strict';

var debug = require('debug')('mojito:logger');

/**
Configures YUI logger to honor the logLevel and logLevelOrder
TODO: This should be done at the low level in YUI.

@method configureLogger
@public
@param {YUI} Y shared YUI instance on the server
**/
module.exports = {
    configureLogger: function (Y) {

        var logLevel = (Y.config.logLevel || 'debug').toLowerCase(),
            logLevelOrder = Y.config.logLevelOrder || [],
            defaultLogLevel = logLevelOrder[0] || 'info',
            isatty = process.stdout.isTTY;

        function log(c, msg, cat, src) {
            var f,
                m = (src) ? src + ': ' + msg : msg;

            // if stdout is bound to the tty, we should try to
            // use the fancy logs implemented by 'yui-log-nodejs'.
            // TODO: eventually YUI should take care of this piece.
            if (isatty && Y.Lang.isFunction(c.logFn)) {
                c.logFn.call(Y, msg, cat, src);
            } else if ((typeof console !== 'undefined') && console.log) {
                f = (cat && console[cat]) ? cat : 'log';
                console[f](msg);
            }
        }

        // one more hack: we need to make sure that base is attached
        // to be able to listen for Y.on.
        Y.use('base');

        if (Y.config.debug) {

            logLevel = (logLevelOrder.indexOf(logLevel) >= 0 ?
                        logLevel : logLevelOrder[0]);

            // logLevel index defines the begining of the logLevelOrder structure
            // e.g: ['foo', 'bar', 'baz'], and logLevel 'bar' 
            // should produce: ['bar', 'baz']
            logLevelOrder = (logLevel ?
                             logLevelOrder.slice(logLevelOrder.indexOf(logLevel)) :
                             []);

            Y.applyConfig({
                useBrowserConsole: false,
                logLevel: logLevel,
                logLevelOrder: logLevelOrder
            });

            // listening for low level log events to filter some of them.
            Y.on('yui:log', function (e) {
                var c = Y.config,
                    cat = e && e.cat && e.cat.toLowerCase();

                // this covers the case Y.log(msg) without category
                // by using the low priority category from logLevelOrder.
                cat = cat || defaultLogLevel;

                // applying logLevel filters
                if (cat && ((c.logLevel === cat) ||
                            (c.logLevelOrder.indexOf(cat) >= 0))) {
                    log(c, e.msg, cat, e.src);
                }
                return true;
            });
        }
    }
};



