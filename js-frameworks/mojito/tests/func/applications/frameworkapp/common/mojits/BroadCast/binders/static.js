/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('BroadCastBinderStatic', function(Y, NAME) {

/**
 * The BroadCastBinderStatic module.
 *
 * @module BroadCastBinderStatic
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
            this.mp = mojitProxy;
            var self = this;
            //Y.log(this.mp);
        },

        /**
         * The binder method, invoked to allow the mojit to attach DOM event
         * handlers.
         *
         * @param node {Node} The DOM node to which this mojit is attached.
         */
        bind: function(node) {
            this.node = node;
            /*var value, id;
            value = this.mp.getFromUrl('test');
            Y.log("**********test key value*********"+value);
            this.node.one('#thisvalue').setContent(value);
            id = this.mp.getId();
            Y.log("**********id**********"+id);
            this.node.one('#thisid').setContent(id);
            var data = {
		        data: "abc"
		    }
            self.mp.render(data, "selfinvoke", function(error, result){
		       Y.log("Here....."+result+error);
        	   self.node.one(".render").append(result);
            });*/
        },

        handleClick: function(evt) {   
            var event, message, child;
            if (evt.currentTarget.get('tagName') === 'BUTTON') {
                event = this.node.one('#event').get('value');
                Y.log("**********event value*********"+event);
                message = this.node.one('#message').get('value');
                Y.log("**********message value*********"+message);
                child = this.node.one('#child').get('value');
                Y.log("**********child value*********"+child);
                if(child === 'all'){
                    Y.log("**********sending to all*********");
                    this.mp.broadcast(event, {message: message});
                }else{
                    this.mp.broadcast(event, {message: message}, {target: {slot: child}});
                }
            }
        }

    };

}, '0.0.1', {requires: ['mojito-client']});
