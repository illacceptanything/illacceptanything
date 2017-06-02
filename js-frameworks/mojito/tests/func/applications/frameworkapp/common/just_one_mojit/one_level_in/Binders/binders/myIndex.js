/*
* Copyright (c) 2011 Yahoo! Inc. All rights reserved.
*/
YUI.add('BindersBindermyIndex', function(Y, NAME) {

/**
 * The BindersBinder module.
 *
 * @module BindersBinder
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
            Y.log('init(' + this.id + ')', 'debug', NAME);
            this.count = 0;
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
                Y.log(this.id + ' clicked', 'debug', NAME);
                this.count++;
                if (this.count === 1)
                {
                    this.node.append('<p id="data">' + this.mojitProxy.data.get('config_data') + ' for the id: ' + this.id +'</p>');
                    this.node.append('<p id="para1">I clicked myself ' + this.count + ' time</p>');
                }
                else
                {
                    this.node.append('<p id="para' + this.count + '">I clicked myself ' + this.count + ' times</p>');
                }
                this.node.addClass('myClass');
            }, this);
        }
    };

}, '0.0.1', {requires: ['node']});
