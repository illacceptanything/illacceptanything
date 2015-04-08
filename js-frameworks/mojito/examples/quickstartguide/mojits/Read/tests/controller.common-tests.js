/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/**
Unit tests in Mojito

Mojito provides a testing framework based on YUI Test that allows you to run unit tests for modules, 
applications, mojit controllers, mojit models, and mojit binders.
For more info, visit: http://developer.yahoo.com/cocktails/mojito/docs/topics/mojito_testing.html

For all assertion functions, visit:
http://yuilibrary.com/yui/docs/api/classes/Test.Assert.html

Controller Tests

A mojit can have one or more controllers that have different affinities. For each controller, you can create create 
a test controller with the same affinity or use controller.common-tests.js, which tests controllers with any affinity. 
For example, controller.server.js can be tested with controller.server-tests.js or controller.common-tests.js.
For more info, visit: http://developer.yahoo.com/cocktails/mojito/docs/topics/mojito_testing.html#controller-tests
**/

/*jslint anon:true, unparam:true*/
/*global YUI, YUITest*/


YUI.add('read-tests', function(Y, NAME) {
    'use strict';

    var suite = new YUITest.TestSuite(NAME),
        controller = null,
        // Shorthand common used variables
        MA = Y.mojito.MockActionContext,
        A = YUITest.Assert,
        M = YUITest.Mock;

    suite.add(new YUITest.TestCase({

        name: 'read tests',

        // setup function is always called before each test runs
        setUp: function() {
            A.isNull(controller);
            // Fetches the controller we are going to test
            controller = Y.mojito.controllers["read"];
            A.isNotNull(controller);
        },

        // tearDown function is always called after each test runs
        tearDown: function() {
            controller = null;
        },

        'test compose method - good with link to prev and next': function() {
            // Sets up mock libraries for testing
            var config = {
                    test: {
                        libs: {
                            lib_md: function(mdContent) {
                                A.areSame("### My Content ###", mdContent);
                                return "<h3> My Content </h3>";
                            }
                        }
                    }
                },
                // Initializes an ActionContext mock object
                ac = new MA({
                    // Declares the addons will be used
                    addons: ["url"],
                    // Declares the models will be used
                    models: ['guide']
                }),
                // Initializes other input arguments for test
                guidemeta = {
                    filename: "myfile"
                },
                data = {
                    title: "mytitle",
                    content: "### My Content ###"
                },
                called = false,
                assertCallback = function(obj) {
                    // Final assertions will be called in this callback function
                    A.areSame("mytitle", obj.title);
                    A.areSame("<h3> My Content </h3>", obj.content);
                    A.areSame("to/prev/link", obj.prev);
                    A.areSame("to/next/link", obj.next);
                    called = true;
                };

            // Sets up expectation objects with addon
            ac.url.expect({
                method: "make",
                args: [M.Value.String, M.Value.String, M.Value.Object],
                run: function(mojit, method, routeParams) {
                    A.areSame('read', mojit);
                    A.areSame('index', method);
                    if (routeParams.filename === "prev_link") {
                        return 'to/prev/link';
                    } else if (routeParams.filename === "next_link") {
                        return 'to/next/link';
                    }
                }
            });

            // Defines a "get" function for model (mojito 0.5.x)
            ac.models.get = function(model_name) {
                return ac.models[model_name];
            };

            // Sets up expectation objects with model
            ac.models["guide"].expect({
                method: "getGuide",
                args: [M.Value.Object, M.Value.Function],
                run: function(guidemeta, cb){
                    A.areSame("myfile", guidemeta.filename, "Guidemeta should be 'myfile'");
                    cb(null, {
                        title: "mytitle",
                        content: "<h1>Header</h1> <p>Paragraph</p>"
                    });
                }
            },{
                method: "getAdjacentGuideFilenames",
                args: [M.Value.String, M.Value.Function],
                run: function(filename, cb){
                    A.areSame("myfile", filename, "Filename should be 'myfile'");
                    cb("prev_link", "next_link");
                }
            });

            // Makes sure init_test is in controller
            A.isNotNull(controller && controller.test && controller.test.init_test);

            // Loads our pre-defined mock libraries
            controller.test.init_test(config);

            // Makes sure the controller method is not null
            A.isNotNull(controller && controller.test && controller.test.compose);

            // Calls the method with pre-defined input arguments
            controller.test.compose(guidemeta, data, ac, assertCallback);

            // Makes sure our final assertion function is called
            A.isTrue(called);
        },


        'test compose method - good with no other links': function() {
            // Sets up mock libraries for testing
            var config = {
                    test: {
                        libs: {
                            lib_md: function(mdContent) {
                                A.areSame("### My Content ###", mdContent);
                                return "<h3> My Content </h3>";
                            }
                        }
                    }
                },
                // Initializes an ActionContext mock object
                ac = new MA({
                    // Declares the models will be used
                    models: ['guide']
                }),
                // Initializes other input arguments for test
                guidemeta = {
                    filename: "myfile"
                },
                data = {
                    title: "mytitle",
                    content: "### My Content ###"
                },
                called = false,
                assertCallback = function(obj) {
                    // Final assertions will be called in this callback function
                    A.areSame("mytitle", obj.title);
                    A.areSame("<h3> My Content </h3>", obj.content);
                    A.isUndefined(obj.prev);
                    A.isUndefined(obj.next);
                    called = true;
                };

            // Defines a "get" function for model (mojito 0.5.x)
            ac.models.get = function(model_name) {
                return ac.models[model_name];
            };

            // Sets up expectation objects with model
            ac.models["guide"].expect({
                method: "getGuide",
                args: [M.Value.Object, M.Value.Function],
                run: function(guidemeta, cb){
                    A.areSame("myfile", guidemeta.filename, "Guidemeta should be 'myfile'");
                    cb(null, {
                        title: "mytitle",
                        content: "<h1>Header</h1> <p>Paragraph</p>"
                    });
                }
            },{
                method: "getAdjacentGuideFilenames",
                args: [M.Value.String, M.Value.Function],
                run: function(filename, cb){
                    A.areSame("myfile", filename, "Filename should be 'myfile'");
                    // Callback with "null"s means no prev/next links
                    cb(null, null);
                }
            });

            // Makes sure init_test is in controller
            A.isNotNull(controller && controller.test && controller.test.init_test);

            // Loads our pre-defined mock libraries
            controller.test.init_test(config);

            // Makes sure the controller method is not null
            A.isNotNull(controller && controller.test && controller.test.compose);

            // Calls the method with pre-defined input arguments
            controller.test.compose(guidemeta, data, ac, assertCallback);

            // Makes sure our final assertion function is called
            A.isTrue(called);
        },

        "test fail method - good? well, it's a fail method ;)": function() {
            // Initializes an ActionContext mock object
            var ac = new MA({
                    // Declares the addons will be used
                    addons: ['intl']
                }),
                // Initializes other input arguments for test
                errorObj = {
                    name: "Test Error",
                    message: "I'm an error!!!"
                };

            // Sets up expectation objects with addon
            ac.intl.expect({
                method: "lang",
                args: [M.Value.String],
                run: function(translatee){
                    A.areSame("ERROR_TITLE", translatee);
                    return "ERROR TITLE!";
                }
            });

            // Sets up tests in ac.done call
            ac.expect({
                method: "done",
                args: [M.Value.Object],
                run: function(obj) {
                    A.areSame("ERROR TITLE!", obj.title);
                    A.areSame("I'm an error!!!", obj.content.message);
                    A.isUndefined(obj.prev);
                    A.isUndefined(obj.next);
                }
            });

            // Makes sure the controller method is not null
            A.isNotNull(controller && controller.test && controller.test.fail);

            // Calls the method with pre-defined input arguments
            controller.test.fail(errorObj, ac);

            // Verify expectations in ac mock. (Not suitable for the models that we define "get" manually)
            ac.verify();
        },

        'test index method - good content': function() {
            // Sets up mock libraries for testing
            var config = {
                    test: {
                        libs: {
                            lib_md: function(mdContent) {
                                A.areSame("### My Content ###", mdContent);
                                return "<h3> My Content </h3>";
                            }
                        }
                    }
                },
                // Initializes an ActionContext mock object
                ac = new MA({
                    // Declares the addons will be used
                    addons: ['params', "url"],
                    // Declares the models will be used
                    models: ['guide']
                });

            // Sets up expectation objects with addon
            ac.url.expect({
                method: "make",
                args: [M.Value.String, M.Value.String, M.Value.Object],
                run: function(mojit, method, routeParams) {
                    // Argument checks
                    A.areSame('read', mojit);
                    A.areSame('index', method);
                    if (routeParams.filename === "prev_link") {
                        return 'to/prev/link';
                    } else if (routeParams.filename === "next_link") {
                        return 'to/next/link';
                    }
                }
            });

            ac.params.expect({
                method: "merged",
                args: [],
                run: function(){
                    return {
                        filename: "myfile"
                    };
                }
            });

            // Defines a "get" function for model (mojito 0.5.x)
            ac.models.get = function(model_name) {
                return ac.models[model_name];
            };

            // Sets up expectation objects with model
            ac.models["guide"].expect({
                method: "getGuide",
                args: [M.Value.Object, M.Value.Function],
                run: function(guidemeta, cb){
                    A.areSame("myfile", guidemeta.filename, "Guidemeta should be 'myfile'");
                    cb(null, {
                        title: "mytitle",
                        content: "### My Content ###"
                    });
                }
            },{
                method: "getAdjacentGuideFilenames",
                args: [M.Value.String, M.Value.Function],
                run: function(filename, cb){
                    A.areSame("myfile", filename, "Filename should be 'myfile'");
                    cb("prev_link", "next_link");
                }
            });

            // Sets up tests in ac.done call
            ac.expect({
                method: "done",
                args: [M.Value.Object],
                run: function(obj) {
                    A.areSame("mytitle", obj.title);
                    A.areSame("<h3> My Content </h3>", obj.content);
                    A.areSame("to/prev/link", obj.prev);
                    A.areSame("to/next/link", obj.next);
                }
            });

            // Makes sure init_test is in controller
            A.isNotNull(controller && controller.test && controller.test.init_test);

            // Loads our pre-defined mock libraries
            controller.test.init_test(config);

            // Makes sure the controller method is not null
            A.isNotNull(controller && controller.index);

            // Running test with mocked ActionContext
            controller.index(ac);
        },


        'test index method - bad content': function() {
            // Initializes an ActionContext mock object
            var ac = new MA({
                    // Declares the addons will be used
                    addons: ['params', "intl"],
                    // Declares the models will be used
                    models: ['guide']
                });

            // Sets up expectation objects with addon
            ac.params.expect({
                method: "merged",
                args: [],
                run: function(){
                    return {
                        filename: "myfile"
                    };
                }
            });
            ac.intl.expect({
                method: "lang",
                args: [M.Value.String],
                run: function(translate) {
                    A.areSame('ERROR_TITLE', translate);
                    return "oh noes!";
                }
            });

            // Defines a "get" function for model (mojito 0.5.x)
            ac.models.get = function(model_name) {
                return ac.models[model_name];
            };

            // Sets up expectation objects with model
            ac.models["guide"].expect({
                method: "getGuide",
                args: [M.Value.Object, M.Value.Function],
                run: function(guidemeta, cb){
                    A.areSame("myfile", guidemeta.filename, "Guidemeta should be 'myfile'");
                    cb("Loading Error mock");
                }
            });

            // Sets up tests in ac.done call
            ac.expect({
                method: "done",
                args: [M.Value.Object],
                run: function(obj) {
                    A.areSame("oh noes!", obj.title);
                    A.areSame("Loading Error mock", obj.content);
                }
            });

            // Makes sure the controller method is not null
            A.isNotNull(controller && controller.index);

            // Running test with mocked ActionContext
            controller.index(ac);
        }

    }));

    // Adds test cases into TestRunner
    YUITest.TestRunner.add(suite);

}, '0.0.1', {requires: [
    'mojito-test',
    'read'
]});
