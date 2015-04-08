/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint node:true, nomen:true*/

'use strict';

/**
Export a middleware error handler.

@return {Object} The handler.
**/
module.exports = function () {
    return function middlewareMojitoHandlerError(err, req, res, next) {
        var statusCode = res.statusCode || 500;
        res.send(statusCode, {
            code: statusCode,
            error: err.message
        });
    };
};
