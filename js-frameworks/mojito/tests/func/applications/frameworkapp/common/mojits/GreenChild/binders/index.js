/*
 * Copyright (c) 2011-2014 Yahoo! Inc. All rights reserved.
 */
YUI.add('GreenChildBinderIndex', function(Y, NAME) {

/**
 * The GreenChildBinderIndex module.
 *
 * @module GreenChildBinderIndex
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
    function GreenChildBinder() {
        Y.Do.after(function () {
            this.mp.unlisten();
        }, this, 'init');
    }
    Y.namespace('mojito.binders')[NAME] = Y.mojito.util.extend(GreenChildBinder, Y.mojito.binders.ColorChildBinderIndex);

}, '0.0.1', {requires: ['mojito-util', 'ColorChildBinderIndex']});
