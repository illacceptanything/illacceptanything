/*
 * Copyright (c) 2011-2014 Yahoo! Inc. All rights reserved.
 */
YUI.add('PurpleChildModel', function(Y, NAME) {

/**
 * The PurpleChildModel module.
 *
 * @module PurpleChildModel
 */

    /**
     * Constructor for the Model class.
     *
     * @class Model
     * @constructor
     */
    function PurpleModel() {
        this.color = 'purple';
    }
    Y.mojito.models[NAME] = Y.mojito.util.extend(PurpleModel, Y.mojito.models.ColorChildModel);


}, '0.0.1', {requires: ['mojito-util', 'ColorChildModel']});
