/*
 * Copyright (c) 2011-2014 Yahoo! Inc. All rights reserved.
 */
YUI.add('GreenChild', function(Y, NAME) {

/**
 * The GreenChild module.
 *
 * @module GreenChild
 */

    /**
     * Constructor for the Controller class.
     *
     * @class Controller
     * @constructor
     */
    Y.namespace('mojito.controllers')[NAME] = Y.mojito.util.extend({}, Y.mojito.controllers.ColorChild);

}, '0.0.1', {requires: ['mojito-util', 'ColorChild', 'GreenChildModel']});
