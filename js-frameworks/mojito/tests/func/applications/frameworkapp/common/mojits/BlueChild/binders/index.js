/*
 * Copyright (c) 2011-2014 Yahoo! Inc. All rights reserved.
 */
YUI.add('BlueChildBinderIndex', function(Y, NAME) {

/**
 * The BlueChildBinderIndex module.
 *
 * @module BlueChildBinderIndex
 */

    /**
     * Constructor for the Binder class.
     *
     * @param mojitProxy {Object} The proxy to allow the binder to interact
     *        with its owning mojit.
     *
     * @class Binder
     * @constructor
     */
    Y.namespace('mojito.binders')[NAME] = Y.mojito.util.extend({}, Y.mojito.binders.ColorChildBinderIndex);

}, '0.0.1', {requires: ['mojito-util', 'ColorChildBinderIndex']});
