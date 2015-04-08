/*
 * Copyright (c) 2011-2014 Yahoo! Inc. All rights reserved.
 */
YUI.add('RedChildBinderIndex', function(Y, NAME) {

/**
 * The RedChildBinderIndex module.
 *
 * @module RedChildBinderIndex
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
    function RedChildBinder() {
        Y.Do.after(function () {
            this.mp.unlisten('notify');
        }, this, 'init');
    }
    Y.namespace('mojito.binders')[NAME] = Y.mojito.util.extend(RedChildBinder, Y.mojito.binders.ColorChildBinderIndex);

}, '0.0.1', {requires: ['mojito-util', 'ColorChildBinderIndex']});
