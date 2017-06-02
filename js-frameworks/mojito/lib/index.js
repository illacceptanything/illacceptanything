/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint node:true, nomen:true*/

/*
 * By convention many node.js applications use an index.js file so we support
 * that convention. Our goal is to redirect the require() to the proper location
 * of the Mojito app/server baseline and to ensure whatever they export is
 * exported to our callers.
 */
process.chdir(__dirname);

module.exports = require('./mojito.js');
