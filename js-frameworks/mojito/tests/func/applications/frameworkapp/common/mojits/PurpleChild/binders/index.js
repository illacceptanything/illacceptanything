/*
 * Copyright (c) 2014 Yahoo! Inc. All rights reserved.
 */
YUI.add('PurpleChildBinderIndex', function(Y, NAME) {

/**
 * The PurpleChildBinderIndex module.
 *
 * @module PurpleChildBinderIndex
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
    Y.namespace('mojito.binders')[NAME] = Y.mojito.util.extend({}, Y.mojito.binders.BlueChildBinderIndex);

}, '0.0.1', {requires: ['mojito-util', 'BlueChildBinderIndex']});
