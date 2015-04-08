/*
 * Copyright (c) 2011-2013, Yahoo! Inc.  All rights reserved.
 * Copyrights licensed under the New BSD License.
 * See the accompanying LICENSE file for terms.
 */

/*jslint nomen:true*/
/*global YUI*/

/**
 * @module ActionContextAddon
 */
YUI.add('mojito-data-addon', function (Y, NAME) {

    'use strict';

    /**
     * <strong>Access point:</strong> <em>ac.data.*</em> and  <em>ac.pageData.*</em>
     * Addon that provides access to the data and pageData models.
     * See also http://developer.yahoo.com/cocktails/mojito/docs/topics/mojito_data.html#sharing-data
     * @submodule Data.common
     */
    function Addon(command, adapter, ac) {
        var data = command.instance.data,
            pageData = adapter.page.data,
            Class = (ac.context && ac.context.runtime !== 'client' ? Y.Model.Vanilla : Y.Model);

        // create data if needed (on the client side, the mojit proxy is responsible)
        ac.data = command.instance.data = (data && data._isYUIModel ? data : new Class(data || {}));
        // creating pageData if needed
        ac.pageData = adapter.page.data = (pageData && pageData._isYUIModel ? pageData : new Class(pageData || {}));
    }

    // The trick here is not to define the plugin namespace,
    // so we can hang data and pageData from ac object directly.

    /**
     * <strong>Access point:</strong> <em>ac.data.*</em>
     *
     * A model for getting and setting data  that is shared by a single mojit
     * controller, binder, and/or template.
     *
     * See also http://developer.yahoo.com/cocktails/mojito/docs/topics/mojito_data.html#sharing-data
     * @class data
     * @namespace Data.common
     * @uses Model.Vanilla
     */
    /**
     * <strong>Access point:</strong> <em>ac.pageData.*</em>
     *
     * A model for getting and setting data that is shared between mojits on a page.
     *
     * See also http://developer.yahoo.com/cocktails/mojito/docs/topics/mojito_data.html#sharing-data
     * @class pageData
     * @namespace Data.common
     * @uses Model.Vanilla
     */

    Y.namespace('mojito.addons.ac').data = Addon;

}, '0.1.0', {requires: [
    'mojito',
    'model-vanilla',
    'model'
]});
