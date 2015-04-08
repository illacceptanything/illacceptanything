/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('CM_NavBinderIndex', function(Y, NAME) {

/**
 * The CM_NavBinderIndex module.
 *
 * @module CM_NavBinderIndex
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
            this.mojitProxy = mojitProxy;
	        this.id = mojitProxy.data.get('id');
	        this.count = 0;
	        this.paraAlertCalled = false;
        },

        /**
         * The binder method, invoked to allow the mojit to attach DOM event
         * handlers.
         *
         * @param node {Node} The DOM node to which this mojit is attached.
         */
        bind: function(node) {
            this.node = node;
            node.on('click', function() {
                this.count++;
                //Y.log(this.id + ' was clicked', 'debug', NAME);
                //Y.log("Type: " + this.mojitProxy.type);
                if (this.paraAlertCalled === true)
                {
                    this.mojitProxy.broadcast('myClickEvent', {message: 'ALERT - Run Run !!', clickCount: this.count, config: this.mojitProxy.data.get('config'), mojitType: this.mojitProxy.type});
                    this.paraAlertCalled = false;
                    //Y.log("Alert message broadcasted");
                }
                else
                {
                    this.mojitProxy.broadcast('myClickEvent', {message: 'Hi News!', clickCount: this.count, config: this.mojitProxy.data.get('config'), mojitType: this.mojitProxy.type});
                    //Y.log("Normal message broadcasted");
                }
	        }, this);
	        node.one('a').on('click', function() {
	            this.paraAlertCalled = true;
	            this.mojitProxy.broadcast('anchorClickEvent');
	        }, this);
        }

    };

}, '0.0.1', {requires: ['mojito-client', 'node']});
