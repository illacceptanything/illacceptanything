/*jslint anon:true, sloppy:true, nomen:true*/
/*global YUI*/
YUI.add('pagelayout-binder-index', function (Y, NAME) {

/**
 * The pagelayout-binder-index module.
 *
 * @module pagelayout-binder-index
 */

    /**
     * Constructor for the PageLayoutBinderIndex class.
     *
     * @class PageLayoutBinderIndex
     * @constructor
     */
    Y.namespace('mojito.binders')[NAME] = {

        /**
         * Binder initialization method, invoked after all binders on the page
         * have been constructed.
         */
        init: function (mojitProxy) {
            this.mojitProxy = mojitProxy;
        },

        /**
         * The binder method, invoked to allow the mojit to attach DOM event
         * handlers.
         *
         * @param node {Node} The DOM node to which this mojit is attached.
         */
        bind: function (node) {
            var me = this;
            this.node = node;
            /**
             * Example code for the bind method:
             *
             * node.all('dt').on('mouseenter', function (evt) {
             *   var dd = '#dd_' + evt.target.get('text');
             *   me.node.one(dd).addClass('sel');
             *
             * });
             * node.all('dt').on('mouseleave', function (evt) {
             *
             *   var dd = '#dd_' + evt.target.get('text');
             *   me.node.one(dd).removeClass('sel');
             *
             * });
             */

            Y.log("bind called", "info", NAME);
            Y.on("domready", function () {
                Y.one("body").addClass("yui3-skin-sam");
            });

            Y.one(".mybody").delegate('click', function() {
                if (!this.ancestor('div').hasClass('collapse')) {
                    this.ancestor('div').addClass('collapse');
                } else {
                    this.ancestor('div').removeClass('collapse');
                }

            }, 'a.min');
        }

    };

}, '0.0.1', {requires: ['event-mouseenter', 'mojito-client', 'node-event-delegate']});
