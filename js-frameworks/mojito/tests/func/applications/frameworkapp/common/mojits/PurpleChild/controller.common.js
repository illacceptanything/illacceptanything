/*
 * Copyright (c) 2014 Yahoo! Inc. All rights reserved.
 */
YUI.add('PurpleChild', function(Y, NAME) {

/**
 * The PurpleChild module.
 *
 * @module PurpleChild
 */

    /**
     * Constructor for the Controller class.
     *
     * @class Controller
     * @constructor
     */
    Y.namespace('mojito.controllers')[NAME] = Y.mojito.util.extend({}, Y.mojito.controllers.RedChild);

}, '0.0.1', {requires: ['mojito-util', 'RedChild', 'PurpleChildModel']});
