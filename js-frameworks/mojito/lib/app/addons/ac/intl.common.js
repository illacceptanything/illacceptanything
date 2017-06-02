/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */


/*jslint anon:true, sloppy:true, nomen: true*/
/*global YUI*/


/**
 * @module ActionContextAddon
 */
YUI.add('mojito-intl-addon', function(Y, NAME) {

    // Overwriting setLang such that yuiActiveLang is not set,
    // this prevents independent requests from interfering with each other.
    // Previously if one request set a valid language, a subsequent request with
    // an invalid language would end up using the previous language.
    var setLang = function (module) {
        //delete Y.Intl._mod(module).yuiActiveLang;
        return Y.Intl.setLang.apply(Y.Intl, arguments);
    };

    /**
     * <strong>Access point:</strong> <em>ac.intl.*</em>
     * Internationalization addon
     * @class Intl.common
     */
    function IntlAddon(command, adapter, ac) {
        this.ac = ac;
    }


    IntlAddon.prototype = {

        namespace: 'intl',

        /**
         * Returns translated string.
         * @method lang
         * @param label {string} Optional. The initial label to be translated. If not provided, returns a copy of all resources.
         * @param args {string|Array|Object} optional parameters for the string
         * @return {string|Object} translated string for label or if no label was provided an object containing all resources.
         */
        lang: function(label, args) {
            var lang = this.ac.instance.closestLang,
                module = this.ac.instance.controller,
                string;

            if (this.setLang(module, lang)) {
                string = Y.Intl.get(module, label);
            }
            if (string && args) {
                // simple string substitution
                return Y.Lang.sub(string, Y.Lang.isString(args) ? [args] : args);
            }
            return string;
        },


        /**
         * Returns local-specified date.
         * @method formatDate
         * @param {Date} date The initial date to be formatted.
         * @return {string} formatted data for language.
         */
        formatDate: function(date) {
            var lang = this.ac.instance.closestLang,
                module = 'datatype-date-format',
                formattedDate;

            if (this.setLang(module, lang, 'en-US')) {
                formattedDate = Y.DataType.Date.format(date, {format: '%x'});
            }
            return formattedDate;
        },


        /*
         * Sets the closest lang available to the specified lang, resorting to the closest
         * lang to the specified default lang or the root lang.
         * @method setLang
         * @param {String} module The module name.
         * @param {String} lang The BCP 47 language tag.
         * @param {String} defaultLang The BCP 47 language tag of the back up lang.
         * @return boolean true if successful, false if not.
         */
        setLang: function (module, lang, defaultLang) {
            var availableLangs = Object.keys(Y.Intl._mod(module));

            // Choose the best language available.
            // Preferably this is the closest lang available to the lang specified,
            // otherwise it is the closest lang to the specified default lang,
            // otherwise it is the root lang.
            lang = Y.Intl.lookupBestLang(lang, availableLangs)
                || (defaultLang && Y.Intl.lookupBestLang(defaultLang, availableLangs))
                || 'yuiRootLang';

            // If none of these are available then this method returns false and
            // this.lang and this.formatDate do not translate, because otherwise the current active
            // lang would be used, which would mean that different requests or mojits can
            // interfere with each other.

            return Y.Intl.setLang(module, lang);
        }
    };

    Y.namespace('mojito.addons.ac').intl = IntlAddon;

}, '0.1.0', {requires: [
    'intl',
    'datatype-date',
    'mojito',
    'mojito-config-addon'
]});

