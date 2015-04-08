/*
* Copyright (c) 2012, Yahoo! Inc.  All rights reserved.
* Copyrights licensed under the New BSD License.
* See the accompanying LICENSE file for terms.
*/

var config = {};

/**
You can customize arrow to use a custom yui seed for the tests by specifing
the seed url here. This is needed when arrow is trying to use an old version
of yui. For some reason, yui 3.6.0 (arrow's default) is failing, so we keep
using 3.4.1.
**/
config.defaultAppSeed = "http://yui.yahooapis.com/3.4.1/build/yui/yui-min.js";

module.exports = config;

