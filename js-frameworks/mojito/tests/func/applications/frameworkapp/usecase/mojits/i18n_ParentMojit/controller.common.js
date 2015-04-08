/*jslint anon:true, sloppy:true, nomen:true*/
YUI.add('i18n_ParentMojit', function(Y, NAME) {

/**
 * The i18n_ParentMojit module.
 *
 * @module i18n_ParentMojit
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
            Y.log("lang="+Y.dump(Y.Intl.get(NAME)),"warn",NAME);
            ac.assets.addCss('./index.css');
            ac.composite.done({
                status: 'Mojito is working.',
                data: { some: ac.intl.lang('some') }
            });
        }

    };

}, '0.0.1', {requires: ['mojito','dump','mojito-composite-addon','mojito-intl-addon','mojito-assets-addon']});
