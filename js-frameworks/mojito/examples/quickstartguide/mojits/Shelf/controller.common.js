/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/**
Controller for Mojio

After an application has been configured to use a mojit, the mojit controller can either do all of the work or 
delegate the work to models and/or views. In the typical case, the mojit controller requests the model to 
retrieve data and then the controller serves that data to the views.
For more info, visit: http://developer.yahoo.com/cocktails/mojito/docs/intro/mojito_mvc.html#controllers
**/

/*global YUI*/

/**
Displays guide titles in a grid of tiles.
@class ShelfController
**/
YUI.add('shelf', function (Y, NAME) {
    'use strict';

    /**
    Separates prepended number (maybe with a dot in the end) from String, if any
    Also trims the leading/tailing spaces for the remaining string

    Example:
         input: "1. My Topic  "
         output: {
            preNum: "1"
            rest: "My Topic"
        }

    @method preNumFromString
    @private
    @param "String" str To be parsed
    @return {Object} object {
                              preNum: number in String
                              rest: remaining string
                            }
    **/
    function preNumFromString(str) {
        var preNum,
            restString;

        //Regex matches number string at beginning (may be leading string)
        preNum = str.match(/^\s*[0-9]+/) || "";

        //Removes the prepended number from string, and dot after if any
        restString = str.replace(preNum, "").replace(/^\.+/, "");

        return {
            //String.match returns an object for single match, so we parse it into String
            preNum: preNum.toString(),
            rest: Y.Lang.trim(restString)
        };
    }

    /**
    Gets the displaying title and filename for all of the guides.
    Displays a tile for each guide with its title, and set up
      a link to pass the guide file name to the Read mojit
    @method index
    @param {ActionContext} ac The action context.
    **/
    function index(ac) {
        // View template data.
        var vudata = {
                // Tile array.
                tiles: []
            },
            processedTitle,
            model = ac.models.get('guide'),
            afterGetGuides = function (guides) {

                // Iterates through each guide to be added to the view template array
                Y.each(guides, function (guide) {
                    // guide.filename contains filefilename
                    // guide.title contains Guide Title

                    // Separates prepending number string from title
                    processedTitle = preNumFromString(guide.title);
                    guide.titleNum = processedTitle.preNum;
                    guide.titleRest = processedTitle.rest;

                    // Creates link for guide to map to Read mojit
                    guide.link = ac.url.make('read', 'index', {
                        'filename': encodeURIComponent(guide.filename)
                    });
                    vudata.tiles.push(guide);
                });

                // Sends view data to Action Context for rendering.
                ac.done(vudata);
            };

        model.getGuides(afterGetGuides);
    }

    /**
    Register index method under controller.
    Keep all other private functions under "test" for unit testing.
    **/
    Y.namespace('mojito.controllers')[NAME] = {
        index: index,
        test: {
            preNumFromString: preNumFromString
        }
    };

}, '0.0.1', {requires: [
    'mojito',
    'mojito-config-addon',
    'mojito-composite-addon',
    'mojito-models-addon',
    'mojito-url-addon',
    'guide-model'
]});
