/**
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */


/*jslint node:true, nomen:true*/

/**
@module mojito-contextualizer
**/

'use strict';

var debug = require('debug')('mojito:middleware:contextualizer'),
    // TODO: review qs vs querystring performance
    qs = require('querystring'),
    url = require('url'),
    CONTEXT_SERVER = 'server',
    DEFAULT_LANG = 'en',
    OPERA_MINI = 'opera-mini',
    IPHONE = 'iphone',
    IPAD = 'ipad',
    ANDROID = 'android',
    IE_MOBILE = 'iemobile',
    PALM = 'palm',
    KINDLE = 'kindle',
    BLACKBERRY = 'blackberry',
    // _device
    REGEX_OPERA_MINI = /opera mini/i,
    REGEX_IPHONE = /ipod|iphone/i,
    REGEX_IPAD = /ipad/i,
    REGEX_ANDROID = /android/i,
    REGEX_IE_MOBILE = /iris|3g_t|windows ce|opera mobi|windows ce; smartphone;|windows ce; iemobile/i,
    REGEX_PALM = /pre\/|palm os|palm|hiptop|avantgo|fennec|plucker|xiino|blazer|elaine/i,
    REGEX_KINDLE = /kindle/i,
    REGEX_BLACKBERRY = /blackberry/i,
    // _language
    REGEX_ACCEPT_LANGUAGE = / *, */,
    REGEX_LANGUAGE_MATCH = /^([a-z]+)-([a-z]+)$/;

/**
@method device
@private
**/
function device(ua, def) {
    // debug('detecting device from UA: ' + ua);

    // TODO: [Issue 74] Remove regex creation within this function scope,
    // and eventually offload to device catalog
    if (REGEX_OPERA_MINI.test(ua)) {
        return OPERA_MINI;
    }
    if (REGEX_IPHONE.test(ua)) {
        return IPHONE;
    }
    if (REGEX_IPAD.test(ua)) {
        return IPAD;
    }
    if (REGEX_ANDROID.test(ua)) {
        return ANDROID;
    }
    if (REGEX_IE_MOBILE.test(ua)) {
        return IE_MOBILE;
    }
    if (REGEX_PALM.test(ua)) {
        return PALM;
    }
    if (REGEX_KINDLE.test(ua)) {
        return KINDLE;
    }
    if (REGEX_BLACKBERRY.test(ua)) {
        return BLACKBERRY;
    }

    return def;
}

/**
@method language
@private
@param {String} al value of the Accept-Language header
@param {String} def default language
**/
function language(al, def) {

    al = (al || '').trim();

    if (!al) {
        return def;
    }

    var list = al.split(REGEX_ACCEPT_LANGUAGE), // accept-language value can have spaces
        chosen,
        matches;

    if (!list[0].length) { // split always returns an array
        return def;
    }

    chosen = list[0];

    // some useragents send "en-us" instead of the more-correct
    // "en-US" (FF3.8.13)
    matches = chosen.match(REGEX_LANGUAGE_MATCH);
    if (matches) {
        chosen = matches[1] + '-' + matches[2].toUpperCase();
    }

    return chosen;
}

/**
@method preferredLanguages
@private
@param {String} lang
@param {String} defaultLang
@return {String} defaultLang
**/
function preferredLanguages(lang, defaultLang) {
    return [lang, defaultLang].join(',');
}

/**
@method RequestContextualizer
@public
@return {Function} express middleware
**/
function RequestContextualizer() {

    var defaultLang;

    return function middlewareMojitoContextualizer(req, res, next) {


        if (!defaultLang && req.app && req.app.mojito) {
            defaultLang = (req.app.mojito.context && req.app.mojito.context.lang) ||
                            DEFAULT_LANG;
        }

        var query = url.parse(req.url, true).query || {};

        if (!req.context) {
            req.context = {};
        }

        req.context.runtime = CONTEXT_SERVER;
        req.context.site = query.site || '';
        // TODO: [Issue 86] add configuration switch to detect device
        req.context.device = query.device ||
            device(req.headers['user-agent'], '');
        req.context.lang = query.lang ||
            language(req.headers['accept-language'], defaultLang);
        req.context.langs = query.langs ||
            preferredLanguages(req.context.lang, defaultLang);
        req.context.region = query.region || '';
        req.context.jurisdiction = query.jurisdiction || '';
        req.context.bucket = query.bucket || '';
        req.context.flavor = query.flavor || '';
        req.context.tz = query.tz || '';

        // debug('detected lang: ' + req.context.lang, 'debug',
        //    'request-contextualizer');
        // debug('detected device: ' + req.context.device, 'debug',
        //    'request-contextualizer');
        next();
    };
}

/**
@type {Function} express middleware
**/
module.exports = RequestContextualizer;
