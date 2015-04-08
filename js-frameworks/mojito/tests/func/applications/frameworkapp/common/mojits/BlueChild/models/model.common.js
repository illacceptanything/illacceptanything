/*
 * Copyright (c) 2011-2014 Yahoo! Inc. All rights reserved.
 */
YUI.add('BlueChildModel', function(Y, NAME) {

/**
 * The BlueChildModel module.
 *
 * @module BlueChildModel
 */

    /**
     * Constructor for the Model class.
     *
     * @class Model
     * @constructor
     */
    function BlueModel() {
        this.color = 'blue';
    }
    Y.mojito.models[NAME] = Y.mojito.util.extend(BlueModel, Y.mojito.models.ColorChildModel);


}, '0.0.1', {requires: ['mojito-util', 'ColorChildModel']});
