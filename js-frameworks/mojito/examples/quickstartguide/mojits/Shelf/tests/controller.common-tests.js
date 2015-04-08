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

/*jslint anon:true, sloppy:true, nomen:true*/
/*global YUI,YUITest*/


YUI.add('shelf-tests', function(Y, NAME) {

    var suite = new YUITest.TestSuite(NAME),
        controller = null,
        // Shorthand common used variables
        MA = Y.mojito.MockActionContext,
        A = YUITest.Assert,
        M = YUITest.Mock;

    suite.add(new YUITest.TestCase({

        name: 'shelf tests',

        // setup function is always called before each test runs
        setUp: function() {
            A.isNull(controller);
            // Fetches the controller we are going to test
            controller = Y.mojito.controllers["shelf"];
            A.isNotNull(controller);
        },

        // tearDown function is always called after each test runs
        tearDown: function() {
            controller = null;
        },

        'test preNumFromString method - String start with Num+dot': function() {
            var testString = "100000. Hello Worldddd\n\n\n",
                funcRetObj;

            // Makes sure the controller method is not null
            A.isNotNull(controller && controller.test && controller.test.preNumFromString);

            // Executes the method, store result
            funcRetObj = controller.test.preNumFromString(testString);

            // Compares stored result with expected output
            A.areSame("100000", funcRetObj.preNum);
            A.areSame("Hello Worldddd", funcRetObj.rest);
        },

        'test preNumFromString method - String start without number at front but dot': function() {
            var testString = ". Hello Worldddd\n\n\n",
                funcRetObj;

            // Makes sure the controller method is not null
            A.isNotNull(controller && controller.test && controller.test.preNumFromString);

            // Executes the method, store result
            funcRetObj = controller.test.preNumFromString(testString);

            // Compares stored result with expected output
            A.areSame("", funcRetObj.preNum);
            A.areSame("Hello Worldddd", funcRetObj.rest);
        },

        'test index method': function() {
            // Initializes an ActionContext mock object
			var ac = new MA({
                // Declares the addons will be used
                addons: ['url'],
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
                    if (routeParams.filename === "file001") {
                        return 'read.html?filename=file001';
                    } else if (routeParams.filename === "file002") {
                        return 'read.html?filename=file002';
                    }
                }
            });

            // Defines a "get" function for model (mojito 0.5.x)
            ac.models.get = function(model_name) {
                return ac.models[model_name];
            };

            // Sets up expectation objects with model
            ac.models["guide"].expect({
				method: "getGuides",
				args: [M.Value.Function],
				run: function(cb){
					cb([{
						filename: "file001",
						title: "title001"
					}, {
						filename: "file002",
						title: "title002"
					}]);
				}
			});

            // Sets up tests in ac.done call
			ac.expect({
				method: "done",
				args: [M.Value.Object],
				run: function(obj) {
					A.areSame(2, obj.tiles.length, "Wrong tile counts.");
					A.areSame("title001", obj.tiles[0].title, "Wrong title");
                    A.areSame("title002", obj.tiles[1].title, "Wrong title");
                    A.areSame("read.html?filename=file001", obj.tiles[0].link, "Wrong link");
                    A.areSame("read.html?filename=file002", obj.tiles[1].link, "Wrong link");
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
    'shelf',
    'oop'
]});
