/*
 * Copyright (c) 2011-2014 Yahoo! Inc. All rights reserved.
 */
YUI.add('RedChildModel', function(Y, NAME) {

/**
 * The RedChildModel module.
 *
 * @module RedChildModel
 */

    /**
     * Constructor for the Model class.
     *
     * @class Model
     * @constructor
     */
    function RedModel() {
        this.color = 'red';
    }
    Y.mojito.models[NAME] = Y.mojito.util.extend(RedModel, Y.mojito.models.ColorChildModel);


}, '0.0.1', {requires: ['mojito-util', 'ColorChildModel']});
