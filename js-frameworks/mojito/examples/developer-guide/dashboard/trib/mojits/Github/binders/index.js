/*jslint anon:true, sloppy:true, nomen:true*/
YUI.add('github-binder-index', function (Y, NAME) {

/**
 * The github-binder-index module.
 *
 * @module github-binder-index
 */

    /**
     * Constructor for the GithubBinderIndex class.
     *
     * @class GithubBinderIndex
     * @constructor
     */

    /** 
     * Refreshes content for the node containing tweets.
     *
     * @method refreshMojit
     * @param {Object} node HTML node containing tweets
     * @param {Object} mp mojitProxy object used to communicate w/ server.
     * @return none
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
            this.node = node;
            var me = this.node,
                mp = this.mojitProxy,
                refreshMojit = function(evt) {
                    var tgt = evt.target;
                    evt.halt();
                    mp.invoke('index', function(err, markup) {
                        if (me) {
                            me.replace(markup);
                        }
                    });
                };
            // Refresh the content when user clicks refresh button.
            Y.one("#github").delegate('click', refreshMojit, 'a.refresh');
        }
    };
}, '0.0.1', {requires: ['event-mouseenter', 'mojito-client']});
