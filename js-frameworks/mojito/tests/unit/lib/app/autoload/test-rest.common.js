/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */
YUI().use('mojito-rest-lib', 'test', function(Y) {

    var suite = new Y.Test.Suite('mojito-rest-lib-tests'),
        A = Y.Assert,
        OA = Y.ObjectAssert;

    suite.add(new Y.Test.Case({

        name: 'deferring methods',

        'test GET call defers to main call function': function() {
            // arrange and mock
            var doRequest = Y.mojito.lib.REST._doRequest;
            var doRequestCalled = false;
            Y.mojito.lib.REST._doRequest = function(url, config) {
                // assert
                A.areSame('GET', config.method, 'wrong method');
                A.areSame('url?params', url, 'wrong url');
                A.areSame('header1', config.headers.header1, 'wrong header');
                A.areSame(10000, config.timeout, 'wrong timeout');
                A.isFunction(config.on.success, 'callback is not a function');
                doRequestCalled = true;
            };

            // act
            Y.mojito.lib.REST.GET('url', 'params', {
                headers: {
                    'header1': 'header1'
                },
                timeout: 10000

            }, 'callback');

            // assert
            A.isTrue(doRequestCalled, 'doRequest never called');

            // replace mocks
            Y.mojito.lib.REST._doRequest = doRequest;
        },

        'test POST call defers to main call function': function() {
            // arrange and mock
            var doRequest = Y.mojito.lib.REST._doRequest;
            var doRequestCalled = false;
            Y.mojito.lib.REST._doRequest = function(url, config) {
                // assert
                A.areSame('POST', config.method, 'wrong method');
                A.areSame('url', url, 'wrong url');
                A.areSame('params', config.data, 'data not added to body');
                doRequestCalled = true;
            };

            // act
            Y.mojito.lib.REST.POST('url', 'params');

            // assert
            A.isTrue(doRequestCalled, 'doRequest never called');

            // replace mocks
            Y.mojito.lib.REST._doRequest = doRequest;
        },

        'test PUT call defers to main call function': function() {
            // arrange and mock
            var doRequest = Y.mojito.lib.REST._doRequest;
            var doRequestCalled = false;
            Y.mojito.lib.REST._doRequest = function(url, config) {
                // assert
                A.areSame('PUT', config.method, 'wrong method');
                A.areSame('url', url, 'wrong url');
                A.areSame('params', config.data, 'data not added to body');
                doRequestCalled = true;
            };

            // act
            Y.mojito.lib.REST.PUT('url', 'params');

            // assert
            A.isTrue(doRequestCalled, 'doRequest never called');

            // replace mocks
            Y.mojito.lib.REST._doRequest = doRequest;
        },

        'test DELETE call defers to main call function': function() {
            // arrange and mock
            var doRequest = Y.mojito.lib.REST._doRequest;
            var doRequestCalled = false;
            Y.mojito.lib.REST._doRequest = function(url, config) {
                // assert
                A.areSame('DELETE', config.method, 'wrong method');
                A.areSame('url', url, 'wrong url');
                doRequestCalled = true;
            };

            // act
            Y.mojito.lib.REST.DELETE('url', 'params', 'config', 'callback');

            // assert
            A.isTrue(doRequestCalled, 'doRequest never called');

            // replace mocks
            Y.mojito.lib.REST._doRequest = doRequest;
        },

        'test HEAD call defers to main call function': function() {
            // arrange and mock
            var doRequest = Y.mojito.lib.REST._doRequest;
            var doRequestCalled = false;
            Y.mojito.lib.REST._doRequest = function(url, config) {
                // assert
                A.areSame('HEAD', config.method, 'wrong method');
                A.areSame('url', url, 'wrong url');
                doRequestCalled = true;
            };

            // act
            Y.mojito.lib.REST.HEAD('url', 'params', 'config', 'callback');

            // assert
            A.isTrue(doRequestCalled, 'doRequest never called');

            // replace mocks
            Y.mojito.lib.REST._doRequest = doRequest;
        }

    }));

    suite.add(new Y.Test.Case({

        name: 'callback',

        'test callback is called without err on Y.io success': function() {
            // arrange
            var cbCalled = false;

            Y.mojito.lib.REST._doRequest = function (url, config) {
                config.on.success();
            };

            // act
            Y.mojito.lib.REST._makeRequest('', '', '', '', function(err) {
                A.isNull(err, 'err object within callback should be null on success');
                cbCalled = true;
            });

            // assert
            A.isTrue(cbCalled, 'success callback never called');
        },

        'test callback is called with proper response object on Y.io success': function() {
            // arrange
            var cbCalled = false;
            Y.mojito.lib.REST._doRequest = function(url, opts) {
                opts.on.success(1234, {
                    status: 200,
                    statusText: 'Success',
                    getResponseHeader: function(key) {
                        A.areSame('hkey', key, 'bad header key');
                        return 'got one header';
                    },
                    getResponseHeaders: function() {
                        return 'got all headers';
                    },
                    getAllResponseHeaders: function() {
                        return 'got all headers';
                    },
                    responseText: 'the response body'
                });
            };

            // act
            Y.mojito.lib.REST._makeRequest('', '', '', '', function(err, resp) {
                A.isFunction(resp.getStatusCode, 'response missing getStatusCode function');
                A.isFunction(resp.getStatusMessage, 'response missing getStatusMessage function');
                A.isFunction(resp.getHeader, 'response missing getHeader function');
                A.isFunction(resp.getHeaders, 'response missing getHeaders function');
                A.isFunction(resp.getBody, 'response missing getBody function');
                A.areSame(200, resp.getStatusCode(), 'bad status code');
                A.areSame('Success', resp.getStatusMessage(), 'bad status message');
                A.areSame('got one header', resp.getHeader('hkey'), 'bad response header value');
                A.areSame('got all headers', resp.getHeaders(), 'bad response headers value');
                A.areSame('the response body', resp.getBody(), 'bad response body');
                cbCalled = true;
            });

            // assert
            A.isTrue(cbCalled, 'success callback never called');
        },

        'test callback is called with err on Y.io failure with proper error status and message': function() {
            // arrange
            var cbCalled = false;
            var error = {status:503, statusText:'Wicked Error'};
            Y.mojito.lib.REST._doRequest = function(url, opts) {
                opts.on.failure(1234 /* tx id */, error);
            };

            // act
            Y.mojito.lib.REST._makeRequest('', '', '', '', function(err, data) {
                A.isObject(err, 'err object was not populated');
                OA.areEqual(error, err, 'wrong error object');
                cbCalled = true;
            });

            // assert
            A.isTrue(cbCalled, 'success callback never called');
        }

    }));

    Y.Test.Runner.add(suite);

});
