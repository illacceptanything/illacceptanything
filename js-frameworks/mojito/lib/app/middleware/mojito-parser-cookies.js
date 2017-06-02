/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */


/*jslint nomen:true, node:true*/

/**
@module moijto-parser-cookies
**/

'use strict';

/**
Export a function which can create a cookie parser.
@return {Object} The newly constructed cookie parser.
**/
module.exports = require('express').cookieParser;
