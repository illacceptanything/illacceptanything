/*
 * Copyright (c) 2011-2014 Yahoo! Inc. All rights reserved.
 */
YUI.add('GreenChildModel', function(Y, NAME) {

/**
 * The GreenChildModel module.
 *
 * @module GreenChildModel
 */

    /**
     * Constructor for the Model class.
     *
     * @class Model
     * @constructor
     */
    function GreenModel() {
        this.color = 'green';
    }
    Y.mojito.models[NAME] = Y.mojito.util.extend(GreenModel, Y.mojito.models.ColorChildModel);


}, '0.0.1', {requires: ['mojito-util', 'ColorChildModel']});
