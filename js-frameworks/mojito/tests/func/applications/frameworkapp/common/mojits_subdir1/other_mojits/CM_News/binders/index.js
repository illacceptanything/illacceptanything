/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('CM_NewsBinderIndex', function(Y, NAME) {

/**
 * The CM_NewsBinderIndex module.
 *
 * @module CM_NewsBinderIndex
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
    Y.namespace('mojito.binders')[NAME] = {

        /**
         * Binder initialization method, invoked after all binders on the page
         * have been constructed.
         */
        init: function(mojitProxy) {
    		var self = this; 
            self.mojitProxy = mojitProxy;
            self.id = mojitProxy.data.get('id');
            //Y.log('init()', 'debug', NAME);
            mojitProxy.listen('myClickEvent', function(evt) {
            	//Y.log(evt);
                //Y.log(self.id + ' heard a click from ' + evt.data.mojitType);
                if (self.node) {
                	self.node.append('<p id="click' + evt.data.clickCount + '">' + self.id + ' heard a click from ' + evt.data.config.id + ' (type: ' + evt.data.mojitType + ') with the data: <b>' + evt.data.message + '</b></p>');
                }
            }, this);
            mojitProxy.listen('anchorClickEvent', function(evt) {
                //Y.log(this.id + ' heard a click from ' + evt.source.id);
                if (self.node) {
                	self.node.addClass('alert');
                }
            }, this);
        },

        /**
         * The binder method, invoked to allow the mojit to attach DOM event
         * handlers.
         *
         * @param node {Node} The DOM node to which this mojit is attached.
         */
        bind: function(node) {
            this.node = node;
        }

    };

}, '0.0.1', {requires: []});
