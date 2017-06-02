/*jslint anon:true, sloppy:true, nomen:true*/
YUI.add('i18n_ChildMojit', function(Y, NAME) {

/**
 * The i18n_ChildMojit module.
 *
 * @module i18n_ChildMojit
 */

    /**
     * Constructor for the Controller class.
     *
     * @class Controller
     * @constructor
     */
    Y.namespace('mojito.controllers')[NAME] = {

        /**
         * Method corresponding to the 'index' action.
         *
         * @param ac {Object} The ActionContext that provides access
         *        to the Mojito API.
         */
        index: function(ac) {
            ac.assets.addCss('./index.css');
            ac.done({
                status: 'Mojito child is working.',
                data: { some: ac.intl.lang('some')}
            });
        }

    };

}, '0.0.1', {requires: ['mojito','dump','mojito-assets-addon','mojito-intl-addon']});
