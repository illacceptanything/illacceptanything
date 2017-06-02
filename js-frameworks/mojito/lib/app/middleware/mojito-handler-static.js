/*
 * Ext JS Connect
 * Copyright(c) 2010 Sencha Inc.
 * MIT Licensed
 *
 * Modified by Yahoo!
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Yahoo! Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*
 * Connect staticProvider middleware adapted for Mojito *
 ********************************************************
 * This was modified to allow load all files from the
 * Mojito development environment instead of one static
 * directory.
 ********************************************************
 */


/*jslint node:true, nomen:true */

/**
NOTE: Will be replaced eventually with modown-yui / modown-static
for loading static resources and handling combo requests

@module mojito-handler-static
**/

'use strict';

/**
Module dependencies.
**/
var debug = require('debug')('mojito:middleware:static'),
    liburl   = require('url'),
    libpath  = require('path'),
    libutil  = require('../../util.js'),
    NAME = 'StaticHandler';

/**
File buffer cache.
**/
var _cache = {};

/**
Check if `req` and response `headers`.

@param {IncomingMessage} req
@param {Object} headers
@return {Boolean}
@private
**/
function modified(req, headers) {
    var modifiedSince = req.headers['if-modified-since'],
        lastModified = headers['Last-Modified'],
        noneMatch = req.headers['if-none-match'],
        etag = headers.ETag,
        etagModified = true,
        dateModified = true;

    // Check If-None-Match
    if (noneMatch && etag && noneMatch === etag) {
        etagModified = false;
    }

    // Check If-Modified-Since
    if (modifiedSince && lastModified) {
        modifiedSince = new Date(modifiedSince);
        lastModified = new Date(lastModified);
        // Ignore invalid dates
        if (!isNaN(modifiedSince.getTime())) {
            if (lastModified <= modifiedSince) {
                dateModified = false;
            }
        }
    }

    return etagModified || dateModified;
}

/**
Return an ETag in the form of size-mtime.

@method etag
@param {Object} data buffer with the content to be flushed
@param {Object} stat filesystem stat for the static file.
@return {String}
@private
**/
function etag(data, stat) {
    // using data.length instead of stat.size to support compilation
    return data.length + '-' + Number(stat.mtime);
}

/**
Respond with 304 "Not Modified".

@method notModified
@param {ServerResponse} res
@param {Object} headers
@private
**/
function notModified(res, originalHeaders) {
    var headers = {},
        field;
    // skip Content-* headers
    // making a copy of the original to avoid currupting the cache
    for (field in originalHeaders) {
        if (originalHeaders.hasOwnProperty(field) && (0 !== field.indexOf('Content'))) {
            headers[field] = originalHeaders[field];
        }
    }
    res.writeHead(304, headers);
    res.end();
}

/*
 * Respond with 403 "Forbidden".
 *
 * @method forbidden
 * @param {ServerResponse} res
 * @api private
 */
function forbidden(res) {
    var body = 'Forbidden';
    res.writeHead(403, {
        'Content-Type': 'text/plain',
        'Content-Length': body.length
    });
    res.end(body);
}

/*
 * Respond with 404 "Not Found".
 *
 * @method notFound
 * @param {ServerResponse} res
 * @api private
 */
function notFound(res) {
    var body = 'Not Found';
    res.writeHead(404, {
        'Content-Type': 'text/plain',
        'Content-Length': body.length
    });
    res.end(body);
}

/*
 * Make the appropiated content type based on a resource.
 *
 * @method makeContentTypeHeader
 * @param {object} details details about the resources.
 * @return {string} content-type
 * @api private
 */
function makeContentTypeHeader(details) {
    return details.mimetype + (details.charset ? '; charset=' +
        details.charset : '');
}

/*
 * Static file server.
 *
 * Options:
 *
 *   - `root`     Root path from which to serve static files.
 *   - `maxAge`   Browser cache maxAge in milliseconds, defaults to 0
 *   - `cache`    When true cache files in memory indefinitely,
 *                until invalidated by a conditional GET request.
 *                When given, maxAge will be derived from this value.
 *
 * @param {Object} options
 * @return {Function}
 * @api private
 */
function staticProvider() {

    var app,
        store,
        appConfig,
        yuiDetails,
        staticDetails,
        options,
        cache,
        maxAge,
        staticPath,
        comboPath;

    if (cache && !maxAge) {
        maxAge = cache;
    }
    maxAge = maxAge || 0;

    function done(req, res, hit) {
        if (!options.forceUpdate && !modified(req, hit.headers)) {
            debug(hit.path + ' was not modified');
            notModified(res, hit.headers);
        } else {
            res.writeHead(200, hit.headers);
            res.end(req.method === 'HEAD' ? undefined : hit.body);
        }
    }

    return function middlewareMojitoHandlerStatic(req, res, next) {

        if (req.method !== 'GET' && req.method !== 'HEAD') {
            next();
            return;
        }

        if (!store && req.app && req.app.mojito) {
            app = req.app;
            store = app.mojito.store;

            appConfig = store.getStaticAppConfig();
            yuiDetails = store.yui.getYUIURLDetails();
            staticDetails = store.getAllURLDetails();

            // Give the store access to the static details mapping so that it
            // can add new staticDetails if resources are added lazily.
            store._staticDetails = staticDetails;

            options = appConfig.staticHandling || {};
            cache = options.cache;
            maxAge = options.maxAge || 0;
            staticPath = libutil.webpath(liburl.resolve('/', (options.prefix || 'static') + '/'));

            comboPath = '/combo~';
        }

        var url = liburl.parse(req.url),
            path = url.pathname,
            files = [],
            file,
            result = [],
            failures = 0,
            counter = 0,
            resource,
            i;

        function tryToFlush() {
            var headers,
                len,
                content,
                j;

            if (counter < files.length) {
                return;
            }
            if (counter === files.length) {
                if (failures) {
                    notFound(res);
                    return;
                }
            }

            // first pass computes total length, so we can make a buffer of the
            // correct size
            len = 0;
            for (j = 0; j < counter; j += 1) {
                len += result[j].content.length;
            }
            content = new Buffer(len);

            // second pass actually fills the buffer
            len = 0;
            for (j = 0; j < counter; j += 1) {
                result[j].content.copy(content, len);
                len += result[j].content.length;
            }

            // Serve the content of the file using buffers
            // Response headers
            headers = {
                'Content-Type': makeContentTypeHeader(result[0].details),
                'Content-Length': content.length,
                'Last-Modified': (options.forceUpdate || !result[0].stat) ?
                        new Date().toUTCString() : result[0].stat.ctime.toUTCString(),
                'Cache-Control': 'public, max-age=' + (maxAge / 1000),
                // Return an ETag in the form of size-mtime.
                'ETag': etag(content, result[0].stat)
            };

            done(req, res, {
                path: path,
                headers: headers,
                body: content
            });
            // adding guard in case tryToFlush is called twice.
            counter = 0;

        }


        function readHandler(index, path) {
            // using the clousure to preserve the binding between
            // the index, path and the actual result
            return function (err, data, stat) {

                counter += 1;
                if (err) {
                    console.error('failed to read ' + path + ' because: ' + err.message);
                    notFound(res);
                } else {
                    debug(path + ' was read from disk');
                    result[index].content = data;
                    result[index].stat = stat;
                    // Cache support
                    if (cache) {
                        _cache[path] = result[index];
                    }
                    // in case we have everything ready
                    tryToFlush();
                }

            };
        }


        // TODO: [Issue 87] we should be able to just remove this, because
        // Mojito closes all bad URLs down.
        // Potentially malicious path
        if (path.indexOf('..') !== -1) {
            forbidden(res);
            return;
        }

        // combo urls are allow as well in a form of:
        // - /combo~foo.js~bar.js
        if (path.indexOf(comboPath) === 0) {
            // no spaces allowed
            files = url.pathname.split('~');
            files.shift(); // removing te first element from the list
        } else if (path.indexOf(staticPath) === 0 || staticDetails[path]) {
            files = [path];
        } else {
            // this is not a static file
            next();
            return;
        }

        for (i = 0; i < files.length; i += 1) {

            file = files[i];

            if (cache && _cache[file]) {

                // Cache hit
                debug(file + ' was read from cache');
                result[i] = _cache[file];

            } else if (staticDetails[file]) {

                // geting an static file
                result[i] = {
                    path: file,
                    details: staticDetails[file]
                };

            } else if (yuiDetails[file]) {

                // getting a yui library file
                result[i] = {
                    path: file,
                    details: yuiDetails[file]
                };

            } else if (i < (files.length - 1)) {

                // Note: we are tolerants with the last file in the combo
                // because some proxies/routers might cut the url, and
                // the loader should be able to recover from that if we send
                // a partial response.
                notFound(res);
                return;

            }

        }

        // async queue implementation
        if (result.length > 0) {

            debug('serving ' + (result.length > 1 ? 'combo' : 'static') +
                  ' path: ' + path);

            for (i = 0; i < result.length; i += 1) {
                if (!result[i].content) {
                    store.getResourceContent(result[i].details, readHandler(i, result[i].path));
                } else {
                    counter += 1;
                }
            }

            // in case we get everything from cache
            tryToFlush();

        } else {

            next();
            return;

        }

    };
}


/**
Export function to create the static handler.

@param {Object} options Custom configuration for the middleware.
@params {string} options.root Root path from which to serve static files.
@params {number} options.maxAge Browser cache maxAge in milliseconds, defaults to 0.
    When given, maxAge will be derived from this value.
@params {boolean} options.cache When true cache files in memory indefinitely,
    until invalidated by a conditional GET request.
@return {Function} express middleware
**/
module.exports = staticProvider;


// These will let us unit test
module.exports._etag = etag;
module.exports._forbidden = forbidden;
module.exports._makeContentTypeHeader = makeContentTypeHeader;
module.exports._modified = modified;
module.exports._notFound = notFound;
module.exports._notModified = notModified;
