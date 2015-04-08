/*
 * Copyright (c) 2011 Yahoo! Inc. All rights reserved.
 */
YUI.add('CM_FooterBinderIndex', function(Y, NAME) {

/**
 * The CM_FooterBinderIndex module.
 *
 * @module CM_FooterBinderIndex
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
	        this.config = mojitProxy.config;
        },

        /**
         * The binder method, invoked to allow the mojit to attach DOM event
         * handlers.
         *
         * @param node {Node} The DOM node to which this mojit is attached.
         */
        bind: function(node) {
			//Y.log('bind() - I am in the bind function', 'debug', NAME);
            var self = this;
            self.node = node;
            var para = this.node.one('p');
            var span = para.one('span').get('innerHTML');
            self.node.one('input').on('click', function(){
                //Y.log("I am in the click function");
                var isRPC = false;
                var runFromServer = gup('run_from_server') || "false";
                isRPC = (runFromServer === "true");
                Y.log("This is isRPC: " + isRPC);
                self.mojitProxy.invoke('index',{
                	params : {
                		url: {
                			times: span,
                			run_from_server: runFromServer
                		}
                	},
                    rpc: isRPC
                },    
				function(error, result){
                	//if (error) console.log("**********************I am getting an error: " + error);
                	//Y.log("******************This is the result: " + result);
                    var newNode = Y.Node.create(result);
                    self.node.replace(newNode);
                });
            }, this);
        }
    };

	var gup = function( name ){
        name = name.replace(/[\[]/,"\\\[").replace(/[\]]/,"\\\]");
        var regexS = "[\\?&]"+name+"=([^&#]*)";
        var regex = new RegExp( regexS );
        var results = regex.exec( window.location.href );
        if( results == null )
          return "";
        else
          return results[1];
      }

}, '0.0.1', {requires: []});
