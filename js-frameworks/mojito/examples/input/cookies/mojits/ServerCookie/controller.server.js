/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

YUI.add('servercookie', function(Y, NAME) {
    
    Y.namespace('mojito.controllers')[NAME] = {
        
        pitch: function(ac) {
            var opts = {
                /*

                see: http://stackoverflow.com/questions/489369/can-i-use-localhost-as-the-domain-when-setting-an-http-cookie
                and edit your /etc/hosts file before the options below will work:

                replace
                127.0.0.1 localhost
                with
                127.0.0.1 mojito-cookie-test.edu

                (You'll also want to change it back when you're done.)

                 */
                domain: 'mojito-cookie-test.edu',
                path: '/',
                expires: new Date(2035, 1, 1)
            };
            ac.cookie.set('city', 'Cleveland', opts);
            ac.cookie.set('name', 'Barbara', opts);
            opts.httpOnly = true;
            ac.cookie.set('noClient', 'it\'s a secret!', opts);
            ac.done();
        },

        catch: function(ac) {
            var cookies = ac.cookie.get(),
                cookieArray = [];
            Y.Object.each(cookies, function(v, k) {
                cookieArray.push({key: k, value: v});
            });
            ac.done({cookies: cookieArray});
        },

        index: function(ac) {
            ac.done();
        }
        
    };
    
}, '0.0.1', {requires: ['mojito-cookie-addon']});
