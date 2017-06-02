/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */


/*jslint anon:true, sloppy:true, nomen:true*/


/**
 * This is an object used as the single pathway for data to leave a mojit
 * action execution. It is used as a component of the ActionContext object,
 * which uses it to call <em>done</em> and <em>flush</em> in order to complete.
 *
 * There are two versions of this object, one for the client, and one for the
 * server. This is the server version, which is more complex than the client
 * version.
 *
 * @class OutputHandler
 * @param {Object} req The Request object.
 * @param {Object} res The Response object.
 * @param {Function} next The next function, which should be invokable.
 * @constructor
 */
var NAME = 'OutputHandler.server',
    libutil = require('util'),
    OutputHandler = function(req, res, next) {
        this.req = req;
        this.res = res;
        this.next = next;
        this.headers = {};
        this.page = {};
    };


OutputHandler.prototype = {

    setLogger: function(logger) {
        this.logger = logger;
    },


    flush: function(data, meta) {
        this._readMeta(meta);
        this._writeHeaders();
        this.res.write(data);
    },


    done: function(data, meta) {
        var name,
            obj,
            size,
            memDebug = {};

        this._readMeta(meta);
        this._writeHeaders();
        if (!data ||
                (typeof data !== 'string' && Object.keys(data).length === 0)) {
            data = '';
        }
        this.res.end(data);
    },


    error: function(err) {
        err = err || new Error('Unknown error occurred');
        if (!err.code) {
            err.code = 500;
        }

        if (err.code === 404) {
            // FUTURE: [Issue 96] default Mojito 404 page
            this.logger.log(err, 'warn', NAME);
        } else {
            this.logger.log(err, 'error', NAME);
        }

        var out = '<html>' +
                '<body><h1>Error: ' + err.code + '</h1>' +
                // The following line that includes the error message has been
                // removed because the Paranoids don't want this data to be
                // revealed in production environments. Once the bug having
                // to do with different development environments has been
                // fixed, we will be able to conditionally display the error
                // details.

                //                "<p>" + err.message + "</p>" +
                '<p>Error details are not available.</p>' +

                '</body>' +
                '</html>';
        // TODO: [Issue 96] If YUI._mojito.DEBUG, add stack.

        this.done(out, {
            http: {
                code: err.code,
                reasonPhrase: err.reasonPhrase,
                headers: {
                    'content-type': 'text/html'
                }
            }
        });
    },


    _readMeta: function(meta) {
        if (!meta || !meta.http) { return; }

        var header;
        for (header in meta.http.headers) {
            if (meta.http.headers.hasOwnProperty(header)) {
                this.headers[header] = meta.http.headers[header];
            }
        }

        this.statusCode = meta.http.code;
        this.reasonPhrase = meta.http.reasonPhrase;
    },


    _writeHeaders: function() {
        if (!this.headersSent) {

            // passing a falsy reason phrase would break, because node uses every non-string arguments[1]
            // as header object/array
            if (this.reasonPhrase && typeof this.reasonPhrase === 'string') {
                this.res.writeHead(this.statusCode || 200, this.reasonPhrase, this.headers);
            } else {
                this.res.writeHead(this.statusCode || 200, this.headers);
            }

            this.headersSent = true;
        }
    }
};


/**
 */
module.exports = OutputHandler;
