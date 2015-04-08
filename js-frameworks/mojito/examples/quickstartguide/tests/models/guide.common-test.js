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

Model Tests

Model tests are largely the same as controller tests, except there can be many of them. The model tests are 
placed in the tests/models directory. You can create multiple model tests or use models.common-tests.js to 
test both server and client models.
For more info, visit: http://developer.yahoo.com/cocktails/mojito/docs/topics/mojito_testing.html#model-tests
**/

YUI.add('guide-model-tests', function (Y, NAME) {
    'use strict';
    var suite = new YUITest.TestSuite(NAME),
    model = null,
    // Shorthand common used variables
    A = YUITest.Assert;

    suite.add(new YUITest.TestCase({

        name: 'guide-model tests',

        // setup function is always called before each test runs
        setUp: function () {
            A.isNull(model);
            // Fetches the model we are going to test
            model = Y.mojito.models["guide-model"];
            A.isNotNull(model);
        },

        // tearDown function is always called after each test runs
        tearDown: function () {
            model = null;
        },

        'test mojit model getPathToGuides()': function(){
            // Sets up mock libraries for testing
            var config = {
                    test: {
                        libs: {
                            lib_path: {
                                join: function(p1, p2, p3) {
                                    A.areSame("dirname", p1);
                                    A.areSame("..", p2);
                                    A.areSame("guides", p3)
                                    return "dirname/../guides/"
                                }
                            },
                            dirname: "dirname"
                        }
                    }
                },
                // Container for returned data
                returnedPath;

            // Makes sure testing modules are present
            A.isNotNull(model && model.test);

            // Makes sure there is an init function to load mock libraries
            A.isFunction(model.test.init_test);

            // Loads our pre-defined mock libraries
            model.test.init_test(config);

            // Makes sure method to be tested presents
            A.isFunction(model.test.getPathToGuides);

            // Stores method output
            returnedPath = model.test.getPathToGuides();

            // Compares stored result with expected value
            A.areSame("dirname/../guides/", returnedPath);
        },

        'test mojit model getGuidesFilenames()': function(){
            // Sets up mock libraries for testing
            var config = {
                    test: {
                        libs: {
                            lib_path: {
                                join: function(p1, p2, p3) {
                                    A.areSame("dirname", p1);
                                    A.areSame("..", p2);
                                    A.areSame("guides", p3)
                                    return "dirname/../guides/"
                                }
                            },
                            lib_fs: {
                                readdir: function(path, callback) {
                                    A.areSame("dirname/../guides/", path);
                                    callback(null, ["file222", "file111"]);
                                }
                            },
                            dirname: "dirname"
                        }
                    }
                },
                // Callback that checks final output of the method
                assertCallback = function (returnedFilenames) {
                    // Compares joined string
                    A.areSame(["file111", "file222"].join(), returnedFilenames.join());
                };

            // Makes sure testing modules are present
            A.isNotNull(model && model.test);

            // Makes sure there is an init function to load mock libraries
            A.isFunction(model.test.init_test);

            // Loads our pre-defined mock libraries
            model.test.init_test(config);

            // Makes sure method to be tested presents
            A.isFunction(model.test.getGuidesFilenames);

            // Executes testing method
            model.test.getGuidesFilenames(assertCallback);
        },


        'test mojit model getGuidesFilenames() - directory reading error': function(){
            // Sets up mock libraries for testing
            var config = {
                    test: {
                        libs: {
                            lib_path: {
                                join: function(p1, p2, p3) {
                                    A.areSame("dirname", p1);
                                    A.areSame("..", p2);
                                    A.areSame("guides", p3)
                                    return "dirname/../guides/"
                                }
                            },
                            lib_fs: {
                                readdir: function(path, callback) {
                                    A.areSame("dirname/../guides/", path);
                                    try {
                                        callback("some error");
                                    } catch (err) {
                                        A.areSame("Read dir error", err.name);
                                        A.areSame("Cannot read directory! Check file system!!", err.message);
                                    }
                                }
                            },
                            dirname: "dirname"
                        }
                    }
                },
                // Callback that checks final output of the method
                assertCallback = function(whatever){
                    // Should never come here!
                    A.fail();
                },
                // We need to overload YUI method for this test case, so we keep a copy of the original YUI method
                oldIntl = Y.Intl,
                testIntl = {
                    get: function(translate_pool) {
                        A.areSame('guide-model', translate_pool);
                        return {
                            READ_DIR_ERROR_NAME: "Read dir error",
                            READ_DIR_ERROR_MESSAGE: "Cannot read directory! Check file system!!"
                        }
                    }
                };

            // Overloads YUI method
            Y.Intl = testIntl;

            // Makes sure testing modules are present
            A.isNotNull(model && model.test);

            // Makes sure there is an init function to load mock libraries
            A.isFunction(model.test.init_test);

            // Loads our pre-defined mock libraries
            model.test.init_test(config);

            // Makes sure method to be tested presents
            A.isFunction(model.test.getGuidesFilenames);

            // Executes testing method
            model.test.getGuidesFilenames(assertCallback);

            // Restores YUI method back
            Y.Intl = oldIntl;
        },

        'test mojit model getGuides()': function(){
            // Sets up mock libraries for testing
            var config = {
                    test: {
                        libs: {
                            lib_path: {
                                join: function(p1, p2, p3) {
                                    if (p3 != null) {
                                        A.areSame("dirname", p1);
                                        A.areSame("..", p2);
                                        A.areSame("guides", p3);
                                        return "dirname/../guides/";
                                    } else {
                                        A.areSame("dirname/../guides/", p1);
                                        if (p2 === "file111.md") {
                                            return "dirname/../guides/file111.md";
                                        } else if (p2 === "file222.md") {
                                            return "dirname/../guides/file222.md";
                                        }
                                    }
                                }
                            },
                            lib_fs: {
                                readdir: function(path, callback) {
                                    A.areSame("dirname/../guides/", path);
                                    callback(null, ["file222.md", "file111.md"]);
                                },
                                readFileSync: function(path, encoding) {
                                    A.areSame("utf8", encoding);
                                    if (path === "dirname/../guides/file111.md"){
                                        return "#HEAD1#\n##HEAD2##";
                                    } else if (path === "dirname/../guides/file222.md"){
                                        return "#HEADA#\n##HEADB##";
                                    }
                                }
                            },
                            dirname: "dirname"
                        }
                    }
                },
                // Callback that checks final output of the method
                assertCallback = function(ret){
                    A.areSame(2, ret.length);
                    A.areSame("file111", ret[0].filename);
                    A.areSame("HEAD1", ret[0].title);
                    A.areSame("file222", ret[1].filename);
                    A.areSame("HEADA", ret[1].title);
                };

            // Makes sure testing modules are present
            A.isNotNull(model && model.test);

            // Makes sure there is an init function to load mock libraries
            A.isFunction(model.test.init_test);

            // Loads our pre-defined mock libraries
            model.test.init_test(config);

            // Makes sure method to be tested presents
            A.isFunction(model.getGuides);

            // Executes testing method
            model.getGuides(assertCallback);
        },

        'test mojit model stripTags()': function(){
            var text = "<html><body><p>Hi There</p></body></html>",
                newText;

            // Makes sure testing modules are present
            A.isNotNull(model && model.test);

            // Makes sure method to be tested presents
            A.isFunction(model.test.stripTags);

            // Stores method output
            newText = model.test.stripTags(text);

            // Compares stored result with expected value
            A.areSame("Hi There", newText);
        },

        'test mojit model getContent()': function(){
            // Sets up mock libraries for testing
            var config = {
                    test: {
                        libs: {
                            lib_path: {
                                join: function(p1, p2, p3) {
                                    if (p3 != null) {
                                        A.areSame("dirname", p1);
                                        A.areSame("..", p2);
                                        A.areSame("guides", p3);
                                        return "dirname/../guides/";
                                    } else {
                                        A.areSame("dirname/../guides/", p1);
                                        A.areSame("myfile", p2);
                                        return "dirname/../guides/myfile";
                                    }
                                }
                            },
                            lib_fs: {
                                readFileSync: function(path, encoding) {
                                    A.areSame("dirname/../guides/myfile", path);
                                    A.areSame("utf8", encoding);
                                    return "my content";
                                }
                            },
                            dirname: "dirname"
                        }
                    }
                },
                // Other input arguments to be tested
                filename = "myfile",
                // Callback that checks final output of the method
                assertCallback = function(ret){
                    A.areSame("my content", ret);
                };

            // Makes sure testing modules are present
            A.isNotNull(model && model.test);

            // Makes sure there is an init function to load mock libraries
            A.isFunction(model.test.init_test);

            // Loads our pre-defined mock libraries
            model.test.init_test(config);

            // Makes sure method to be tested presents
            A.isFunction(model.test.getContent);

            // Executes testing method
            model.test.getContent(filename, assertCallback);
        },

        'test mojit model checkFilename() - fs.exists': function(){
            // Sets up mock libraries for testing
            var config = {
                    test: {
                        libs: {
                            lib_path: {
                                join: function(p1, p2, p3) {
                                    if (p3 != null) {
                                        A.areSame("dirname", p1);
                                        A.areSame("..", p2);
                                        A.areSame("guides", p3);
                                        return "dirname/../guides/";
                                    } else {
                                        A.areSame("dirname/../guides/", p1);
                                        A.areSame("myfile", p2);
                                        return "dirname/../guides/myfile";
                                    }
                                }
                            },
                            lib_fs: {
                                exists: function(path, cb) {
                                    A.areSame("dirname/../guides/myfile", path);
                                    cb(true);
                                }
                            },
                            dirname: "dirname"
                        }
                    }
                },
                // Other input arguments to be tested
                filename = "myfile",
                // Callback that checks final output of the method
                assertCallback = function(ret){
                    A.isTrue(ret);
                };

            // Makes sure testing modules are present
            A.isNotNull(model && model.test);

            // Makes sure there is an init function to load mock libraries
            A.isFunction(model.test.init_test);

            // Loads our pre-defined mock libraries
            model.test.init_test(config);

            // Makes sure method to be tested presents
            A.isFunction(model.test.checkFilename);

            // Executes testing method
            model.test.checkFilename(filename, assertCallback);
        },

        'test mojit model checkFilename() - path.exists': function(){
            // Sets up mock libraries for testing
            var config = {
                    test: {
                        libs: {
                            lib_path: {
                                join: function(p1, p2, p3) {
                                    if (p3 != null) {
                                        A.areSame("dirname", p1);
                                        A.areSame("..", p2);
                                        A.areSame("guides", p3);
                                        return "dirname/../guides/";
                                    } else {
                                        A.areSame("dirname/../guides/", p1);
                                        A.areSame("myfile", p2);
                                        return "dirname/../guides/myfile";
                                    }
                                },
                                exists: function(path, cb) {
                                    A.areSame("dirname/../guides/myfile", path);
                                    cb(true);
                                }
                            },
                            lib_fs: {},
                            dirname: "dirname"
                        }
                    }
                },
                // Other input arguments to be tested
                filename = "myfile",
                // Callback that checks final output of the method
                assertCallback = function(ret){
                    A.isTrue(ret);
                };

            // Makes sure testing modules are present
            A.isNotNull(model && model.test);

            // Makes sure there is an init function to load mock libraries
            A.isFunction(model.test.init_test);

            // Loads our pre-defined mock libraries
            model.test.init_test(config);

            // Makes sure method to be tested presents
            A.isFunction(model.test.checkFilename);

            // Executes testing method
            model.test.checkFilename(filename, assertCallback);
        },

        'test mojit model getGuide() - good': function(){
            // Sets up mock libraries for testing
            var config = {
                    test: {
                        libs: {
                            lib_path: {
                                join: function(p1, p2, p3) {
                                    if (p3 != null) {
                                        A.areSame("dirname", p1);
                                        A.areSame("..", p2);
                                        A.areSame("guides", p3);
                                        return "dirname/../guides/";
                                    } else {
                                        A.areSame("dirname/../guides/", p1);
                                        A.areSame("myfile.md", p2);
                                        return "dirname/../guides/myfile.md";
                                    }
                                }
                            },
                            lib_fs: {
                                exists: function(path, cb) {
                                    A.areSame("dirname/../guides/myfile.md", path, "Bad path in fs.exists");
                                    cb(true);
                                },
                                readFileSync: function(path, encoding, cb) {
                                    A.areSame("dirname/../guides/myfile.md", path, "Bad path in readFile");
                                    A.areSame("utf8", encoding);
                                    return "#Main Header#\n### My Sub Header ###";
                                }
                            },
                            dirname: "dirname"
                        }
                    }
                },
                // Other input arguments to be tested
                guidemeta = {
                    filename: "myfile"
                },
                // Callback that checks final output of the method
                assertCallback = function(err, ret){
                    A.isNull(err);
                    A.areSame("Main Header", ret.title);
                    A.areSame("### My Sub Header ###", ret.content);
                };

            // Makes sure testing modules are present
            A.isNotNull(model && model.test);

            // Makes sure there is an init function to load mock libraries
            A.isFunction(model.test.init_test);

            // Loads our pre-defined mock libraries
            model.test.init_test(config);

            // Makes sure method to be tested presents
            A.isFunction(model.getGuide);

            // Executes testing method
            model.getGuide(guidemeta, assertCallback);
        },

        'test mojit model getGuide() - empty guide': function(){
            // Sets up mock libraries for testing
            var config = {
                    test: {
                        libs: {
                            lib_path: {
                                join: function(p1, p2, p3) {
                                    if (p3 != null) {
                                        A.areSame("dirname", p1);
                                        A.areSame("..", p2);
                                        A.areSame("guides", p3);
                                        return "dirname/../guides/";
                                    } else {
                                        A.areSame("dirname/../guides/", p1);
                                        A.areSame("myfile.md", p2);
                                        return "dirname/../guides/myfile.md";
                                    }
                                }
                            },
                            lib_fs: {
                                exists: function(path, cb) {
                                    A.areSame("dirname/../guides/myfile.md", path, "Bad path in fs.exists");
                                    cb(true);
                                },
                                readFileSync: function(path, encoding, cb) {
                                    A.areSame("dirname/../guides/myfile.md", path, "Bad path in readFile");
                                    A.areSame("utf8", encoding);
                                    // Return empty string!
                                    return "";
                                }
                            },
                            dirname: "dirname"
                        }
                    }
                },
                // Other input arguments to be tested
                guidemeta = {
                    filename: "myfile"
                },
                // Callback that checks final output of the method
                assertCallback = function(err, ret){
                    A.isNull(err);
                    A.areSame("untitled", ret.title);
                    A.areSame("", ret.content);
                };

            // Makes sure testing modules are present
            A.isNotNull(model && model.test);

            // Makes sure there is an init function to load mock libraries
            A.isFunction(model.test.init_test);

            // Loads our pre-defined mock libraries
            model.test.init_test(config);

            // Makes sure method to be tested presents
            A.isFunction(model.getGuide);

            // Executes testing method
            model.getGuide(guidemeta, assertCallback);
        },

        'test mojit model getGuide() - bad file checking': function(){
            // Sets up mock libraries for testing
            var config = {
                    test: {
                        libs: {
                            lib_path: {
                                join: function(p1, p2, p3) {
                                    if (p3 != null) {
                                        A.areSame("dirname", p1);
                                        A.areSame("..", p2);
                                        A.areSame("guides", p3);
                                        return "dirname/../guides/";
                                    } else {
                                        A.areSame("dirname/../guides/", p1);
                                        A.areSame("myfile.md", p2);
                                        return "dirname/../guides/myfile.md";
                                    }
                                }
                            },
                            lib_fs: {
                                exists: function(path, cb) {
                                    A.areSame("dirname/../guides/myfile.md", path, "Bad path in fs.exists");
                                    cb(false);
                                }
                            },
                            dirname: "dirname"
                        }
                    }
                },
                // We need to overload YUI method for this test case, so we keep a copy of the original YUI method
                oldIntl = Y.Intl,
                testIntl = {
                    get: function(translate_pool) {
                        A.areSame('GuideModel', translate_pool);
                        return {
                            ERROR_CONTENT_FETCH: "Ooo, could not fetch content for"
                        }
                    }
                },
                // Other input arguments to be tested
                guidemeta = {
                    filename: "myfile"
                },
                // Callback that checks final output of the method
                assertCallback = function(err){
                    A.areSame("Ooo, could not fetch content for myfile.md", err);
                };

            // Overloads YUI method
            Y.Intl = testIntl;

            // Makes sure testing modules are present
            A.isNotNull(model && model.test);

            // Makes sure there is an init function to load mock libraries
            A.isFunction(model.test.init_test);

            // Loads our pre-defined mock libraries
            model.test.init_test(config);

            // Makes sure method to be tested presents
            A.isFunction(model.getGuide);

            // Executes testing method
            model.getGuide(guidemeta, assertCallback);

            // Restores YUI method back
            Y.Intl = oldIntl;
        },

        'test mojit model getAdjacentGuideFilenames() - good readdir with multiple guides': function(){
            // Sets up mock libraries for testing
            var config = {
                    test: {
                        libs: {
                            lib_path: {
                                join: function(p1, p2, p3) {
                                    A.areSame("dirname", p1);
                                    A.areSame("..", p2);
                                    A.areSame("guides", p3);
                                    return "dirname/../guides";
                                }
                            },
                            lib_fs: {
                                readdir: function(path, callback) {
                                    A.areSame("dirname/../guides", path);
                                    callback(null, ["file222.md", "not_md","file111.md", "file333.md"]);
                                }
                            },
                            dirname: "dirname"
                        }
                    }
                },
                // We need to overload YUI method for this test case, so we keep a copy of the original YUI method
                oldFilter = Y.Array.filter,
                testFilter = function(oldArray, filterFunc) {
                    var newArray = [],
                        i;
                    for (i = 0; i < oldArray.length; i++) {
                        if (filterFunc(oldArray[i])) {
                            newArray.push(oldArray[i]);
                        }
                    };
                    return newArray;
                },
                oldIndexOf = Y.Array.indexOf,
                testIndexOf = function(oldArray, target) {
                    // Simple indexOf function specifically to the test
                    var newArray = [],
                        i;
                    for (i = 0; i < oldArray.length; i++) {
                        if (oldArray[i] === target) {
                            return i;
                        }
                    };
                    return -1;
                },
                // Other input arguments to be tested
                filename = "file222",
                // Callback that checks final output of the method
                assertCallback = function(prevLink, nextLink){
                    A.areSame("file111", prevLink);
                    A.areSame("file333", nextLink);
                };

            // Overloads YUI method
            Y.Array.filter = testFilter;
            Y.Array.indexOf = testIndexOf;

            // Makes sure testing modules are present
            A.isNotNull(model && model.test);

            // Makes sure there is an init function to load mock libraries
            A.isFunction(model.test.init_test);

            // Loads our pre-defined mock libraries
            model.test.init_test(config);

            // Makes sure method to be tested presents
            A.isFunction(model.getAdjacentGuideFilenames);

            // Executes testing method
            model.getAdjacentGuideFilenames(filename, assertCallback);

            // Restores YUI method back
            Y.Array.filter = oldFilter;
            Y.Array.indexOf = oldIndexOf;
        },

        'test mojit model getAdjacentGuideFilenames() - current file is the only valid guide': function(){
            // Sets up mock libraries for testing
            var config = {
                    test: {
                        libs: {
                            lib_path: {
                                join: function(p1, p2, p3) {
                                    A.areSame("dirname", p1);
                                    A.areSame("..", p2);
                                    A.areSame("guides", p3);
                                    return "dirname/../guides";
                                }
                            },
                            lib_fs: {
                                readdir: function(path, callback) {
                                    A.areSame("dirname/../guides", path);
                                    callback(null, ["file222.md", "not_md"]);
                                }
                            },
                            dirname: "dirname"
                        }
                    }
                },
                // We need to overload YUI method for this test case, so we keep a copy of the original YUI method
                oldFilter = Y.Array.filter,
                testFilter = function(oldArray, filterFunc) {
                    var newArray = [],
                        i;
                    for (i = 0; i < oldArray.length; i++) {
                        if (filterFunc(oldArray[i])) {
                            newArray.push(oldArray[i]);
                        }
                    };
                    return newArray;
                },
                // Other input arguments to be tested
                filename = "file222",
                // Callback that checks final output of the method
                assertCallback = function(prevLink, nextLink){
                    A.isNull(prevLink);
                    A.isNull(nextLink);
                };

            // Overloads YUI method
            Y.Array.filter = testFilter;

            // Makes sure testing modules are present
            A.isNotNull(model && model.test);

            // Makes sure there is an init function to load mock libraries
            A.isFunction(model.test.init_test);

            // Loads our pre-defined mock libraries
            model.test.init_test(config);

            // Makes sure method to be tested presents
            A.isFunction(model.getAdjacentGuideFilenames);

            // Executes testing method
            model.getAdjacentGuideFilenames(filename, assertCallback);

            // Restores YUI method back
            Y.Array.filter = oldFilter;
        },

        'test mojit model getAdjacentGuideFilenames() - current file not existed in guides': function(){
            // Sets up mock libraries for testing
            var config = {
                    test: {
                        libs: {
                            lib_path: {
                                join: function(p1, p2, p3) {
                                    A.areSame("dirname", p1);
                                    A.areSame("..", p2);
                                    A.areSame("guides", p3);
                                    return "dirname/../guides";
                                }
                            },
                            lib_fs: {
                                readdir: function(path, callback) {
                                    A.areSame("dirname/../guides", path);
                                    callback(null, ["not_md","file111.md", "file333.md"]);
                                }
                            },
                            dirname: "dirname"
                        }
                    }
                },
                // We need to overload YUI method for this test case, so we keep a copy of the original YUI method
                oldFilter = Y.Array.filter,
                testFilter = function(oldArray, filterFunc) {
                    var newArray = [],
                        i;
                    for (i = 0; i < oldArray.length; i++) {
                        if (filterFunc(oldArray[i])) {
                            newArray.push(oldArray[i]);
                        }
                    };
                    return newArray;
                },
                oldIndexOf = Y.Array.indexOf,
                testIndexOf = function(oldArray, target) {
                    // Simple indexOf function specifically to the test
                    var newArray = [],
                        i;
                    for (i = 0; i < oldArray.length; i++) {
                        if (oldArray[i] === target) {
                            return i;
                        }
                    };
                    return -1;
                },
                // Other input arguments to be tested
                filename = "file222",
                // Callback that checks final output of the method
                assertCallback = function(prevLink, nextLink){
                    A.isNull(prevLink);
                    A.isNull(nextLink);
                };

            // Overloads YUI method
            Y.Array.filter = testFilter;

            // Makes sure testing modules are present
            A.isNotNull(model && model.test);

            // Makes sure there is an init function to load mock libraries
            A.isFunction(model.test.init_test);

            // Loads our pre-defined mock libraries
            model.test.init_test(config);

            // Makes sure method to be tested presents
            A.isFunction(model.getAdjacentGuideFilenames);

            // Executes testing method
            model.getAdjacentGuideFilenames(filename, assertCallback);

            // Restores YUI method back
            Y.Array.filter = oldFilter;
        }

    }));

    // Adds test cases into TestRunner
    YUITest.TestRunner.add(suite);

}, '0.0.1', {requires: ['mojito-test', 'guide-model']});
