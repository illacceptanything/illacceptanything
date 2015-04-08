/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/**
Model for Mojito

Models are intended to closely represent business logic entities and contain code that accesses and persists 
data. Mojito lets you create one or more models at the application and mojit level that can be accessed from 
controllers.
For more info, visit: http://developer.yahoo.com/cocktails/mojito/docs/intro/mojito_mvc.html#models
**/

/*jslint nomen: true, stupid: true, regexp: true*/
/*global YUI*/

/**
Fetches guide file names, titles, and contents via accessing file system.
Used by @Shelf and @Read mojits.
@class GuideModel
**/
YUI.add('guide-model', function (Y, NAME) {
    'use strict';
    var LIB_PATH,// Nodejs path library
        LIB_FS,// Nodejs file system library
        DIRNAME;// Path to the current directory

    /**
    Is used for library initializations in this example application.

    @method init
    @param {Object} config The configuration object.
    **/
    function init(config) {
        LIB_PATH = require('path');
        LIB_FS = require('fs');
        // In Node.js, __dirname contains the path to the current file.
        DIRNAME = __dirname;
    }

    /**
    Is used for library initializations under test environment.

    @method init_test
    @private
    @param {Object} config The configuration object.
    **/
    function init_test(config) {
        // Mocking the library for test purposes.
        // Unit tests are run using "mojito test app app_root/".
        // Make sure the attributes are set up correctly, otherwise don't set at all
        if (config && config.test && config.test.libs) {
            LIB_PATH = config.test.libs.lib_path;
            LIB_FS = config.test.libs.lib_fs;
            DIRNAME = config.test.libs.dirname;
        }
    }

    /**
    Strips out HTML tags from the content string provided.

    Example:
     Input: "<html><body><p>Foo</p></body></html>"
     Output: "Foo"

    @method stripTags
    @private
    @param {String} content The content.
    @return {String} Plain text.
    **/
    function stripTags(content) {
        // Regex that removes HTML tags
        return Y.Lang.trim(content.replace(/<\/?\w+[\s\S]*?>/gmi, ' '));
    }

    /**
    Returns the path to the data source.
    @method getPathToGuides
    @private
    @return {String} path to the data set.
    **/
    function getPathToGuides() {
        // Use LIB_PATH.join() to join path with system default folder symbol.
        return LIB_PATH.join(DIRNAME, '..', 'guides');
    }

    /**
    Gets the filenames of all guides and sort by alphabetical order.
    return callback([Array] An array with guide titles in string.)
    @private
    @method getGuidesFilenames
    @param {Function} callback The callback function to invoke.
    **/
    function getGuidesFilenames(callback) {
        var afterReaddir = function (err, dirArray) {
                if (err) {
                    throw {
                        // Localize program generated text. See .js files under lang/
                        name: Y.Intl.get('guide-model').READ_DIR_ERROR_NAME,
                        message: Y.Intl.get('guide-model').READ_DIR_ERROR_MESSAGE
                    };
                }
                // Sends back result
                callback(dirArray.sort(function (item1, item2) {
                    // Sorted by file name (alphabetical order)
                    return item1.localeCompare(item2);
                }));
            };
        // Use LIB_FS.readdir(path) to read all filenames for the guides
        //   and returns them into an array
        LIB_FS.readdir(getPathToGuides(), afterReaddir);
    }

    /**
    Checks the existence of a file.
    @method checkFilename
    @private
    @param {String} filename.
    @param {Function} callback Function to callback.
    **/
    function checkFilename(filename, callback) {
        // Generates path to the guide
        var path = LIB_PATH.join(getPathToGuides(), filename);

        // Use LIB_(FS|PATH).exists(filepath, callback) that will directly check the file existence
        //   and send result to callback
        // For older node version, exists() belongs to path library
        if (LIB_FS.exists) {
            LIB_FS.exists(path, callback);
        } else {
            LIB_PATH.exists(path, callback);
        }
    }

    /**
    Synchronously opens a file and return its content.
    @method getContent
    @private
    @param {String} filename Name of the file.
    @return {String} file content
    **/
    function getContent(filename) {
        // Generates path to the guide
        var path = LIB_PATH.join(getPathToGuides(), filename);

        // Use LIB_FS.readFileSync(filepath, callback) and return file content. 'utf8' encoding will make sure
        // we get back the result as String
        return LIB_FS.readFileSync(path, 'utf8');
    }

    /**
    Extracts the very first Header element from content.

    Example:
     Input:
      "#Foo#\n
      #Bar#"
     Output:
      {
       title: "Foo"
       content: "#Bar#"
      }

    @method separateGuideTitleFromContent
    @private
    @param {String} guide content.
    @return {Object} guide title and remaining content.
    */
    function separateGuideTitleFromContent(content) {
        // Grab the first line in (any) header format (#+ in markdowns)
        var title = (content.match(/\s*#+.*\s*/) && content.match(/\s*#+.*\s*/)[0]) || "untitled",
            // Set up the rest of content by extracting the title from content.
            restContent = content.replace(title, "");

        return {
            // Extract heading content (Without any # or newline)
            title: title.replace(/#+|\n/g, ""),
            content: restContent
        };
    }

    /**
    Gets filenames and titles of all guides from file system.
    return callback([{
                     filename: filename of the guide without .md
                     title: title of the guide
                    }, {...}, ...])

    @method getGuides
    @param {Function} callback The callback function to send in guides.
    **/
    function getGuides(callback) {
        var returningArray = [],
            afterGetAllFilenames = function (filenames) {
                // Iterates through each filename
                Y.each(filenames, function (filename) {
                    var guide = {},
                        content;

                    // Only cares about the .md files
                    if (filename.match(/\.md$/)) {
                        guide.filename = filename.replace(/\.md$/, "");
                        // Fetches the content, but we only need its title
                        content = stripTags(getContent(filename));
                        guide.title = separateGuideTitleFromContent(content).title;
                        // Pushes to the resulting array
                        returningArray.push(guide);
                    }
                });

                // Sends back result array
                callback(returningArray);
            };

        // Gets all filenames in an array
        getGuidesFilenames(afterGetAllFilenames);
    }

    /**
    Gets content of the .md file.
    return callback(error, {
                            title: guide title,
                            content: guide content
                            })

    @method getGuide
    @param {Object} guidemeta Metadata for the selected feed.
    @param {Function} callback The callback function to invoke.
    **/
    function getGuide(guidemeta, callback) {
        var filename = guidemeta.filename + ".md", // .md file name
            fileContent,
            fileObj,
            afterCheckFile = function (good) {
                var error = null;
                if (good) {
                    // Starts getting the content
                    fileContent = stripTags(getContent(filename));

                    // Separate the title from content
                    fileObj = separateGuideTitleFromContent(fileContent);

                    // Sends back result
                    callback(error, fileObj);
                } else {
                    // Localizes program generated text. See .js files under lang/
                    error = Y.Intl.get('GuideModel').ERROR_CONTENT_FETCH + ' ' + filename;
                    // Sends back error
                    callback(error);
                }
            };

        // Check the existence of the file
        checkFilename(filename, afterCheckFile);
    }

    /**
    Finds the links to previous and next guide.
    return callback(Filename of previous guide,
                    Filename of next guide)

    @method getAdjacentGuideFilenames
    @param {String} filename Exact filename from file system.
    @param {Function} callback The callback function to invoke.
    */
    function getAdjacentGuideFilenames(filename, callback) {
        var fullFilename,
            afterGetAllFilenames = function (allFilenames) {
                var currentIndex,
                    prevIndex,
                    nextIndex,
                    // Filter to keep only .md files left
                    filteredFilenames = Y.Array.filter(allFilenames, function (filename) {
                        return filename.match(/\.md$/);
                    }),
                    numOfGuides = filteredFilenames.length;

                if (numOfGuides <= 1) {
                    // Only 1 element, no prev and next
                    callback(null, null);
                } else {
                    // Find the index of the current filename
                    currentIndex = Y.Array.indexOf(filteredFilenames, fullFilename);

                    if (currentIndex < 0) {
                        // Filename not found!!
                        callback(null, null);
                    } else {
                        // Fill in previous and next index
                        prevIndex = currentIndex > 0 ? currentIndex - 1 : numOfGuides - 1;
                        nextIndex = (currentIndex + 1) % numOfGuides;

                        // Passback the filenames of the adjacent guides
                        callback(filteredFilenames[prevIndex].replace(/\.md$/, ""), filteredFilenames[nextIndex].replace(/\.md$/, ""));
                    }
                }
            };

        // Restore filename with ".md"
        fullFilename = filename + ".md";

        // Gets filenames of all guides
        getGuidesFilenames(afterGetAllFilenames);
    }

    /**
    Register getGuide, getGuides, getAdjacentGuideFilenames, and init methods under controller.
    "init" is called before every model method gets called
    Keep all other private functions under "test" for unit testing.
    **/
    Y.mojito.models[NAME] = {
        getGuide: getGuide,
        getGuides: getGuides,
        getAdjacentGuideFilenames: getAdjacentGuideFilenames,
        init: init,
        test: {
            init_test: init_test,
            getPathToGuides: getPathToGuides,
            checkFilename: checkFilename,
            getContent: getContent,
            stripTags: stripTags,
            separateGuideTitleFromContent: separateGuideTitleFromContent,
            getGuidesFilenames: getGuidesFilenames
        }
    };

}, '0.0.1', {requires: [
    'mojito',
    'intl'
]});
